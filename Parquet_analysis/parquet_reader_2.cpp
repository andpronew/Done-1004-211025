// parquet_reader.cpp
// Row-group columnar reader (zero-copy views; batched decode; optional column selection; chronological file order)
// Build: g++ -std=gnu++17 -O2 parquet_reader.cpp -lparquet -larrow -lzstd
#include <parquet/api/reader.h>
#include <parquet/schema.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <cstring>
#include <utility>
#include <vector>

using namespace std;
namespace fs = filesystem;

// ======== Zero-copy columnar views (valid until next() is called) ========

struct TopColsView
{
  const int64_t* ts     = nullptr;
  const int64_t* askPx  = nullptr;
  const int64_t* askQty = nullptr;
  const int64_t* bidPx  = nullptr;
  const int64_t* bidQty = nullptr;
  const int64_t* valu   = nullptr;
  size_t n = 0;
};

struct DeltaColsView
{
  const int64_t* ts        = nullptr;
  const int64_t* firstId   = nullptr;
  const int64_t* lastId    = nullptr;
  const int64_t* eventTime = nullptr;
  size_t n = 0;

  const uint32_t* ask_off = nullptr;
  const int64_t*  ask_px  = nullptr;
  const int64_t*  ask_qty = nullptr;

  const uint32_t* bid_off = nullptr;
  const int64_t*  bid_px  = nullptr;
  const int64_t*  bid_qty = nullptr;
};

// ======== Column selection ========

struct TopSelect
{
  bool ts     = true;  // returned in view (ts is always read for filtering)
  bool askPx  = true;
  bool askQty = true;
  bool bidPx  = true;
  bool bidQty = true;
  bool valu   = true;
};

struct DeltaSelect
{
  bool ts        = true;  // returned in view (ts is always read for filtering)
  bool firstId   = true;
  bool lastId    = true;
  bool eventTime = true;

  bool ask_px  = true;
  bool ask_qty = true;
  bool bid_px  = true;
  bool bid_qty = true;
};

// ======== Low-level batched cursor ========

struct Entry
{
  int16_t def = 0, rep = 0;
  bool has_value = false;
  int64_t value = 0;
  bool valid = false;
};

struct Int64Cursor
{
  shared_ptr<parquet::ColumnReader> holder;
  parquet::Int64Reader* r = nullptr;
  int16_t max_def = 0, max_rep = 0;
  bool eof = false;

  bool has_pending = false;
  Entry pending;

  static constexpr int64_t BATCH = 65536;
  vector<int16_t> defbuf, repbuf;
  vector<int64_t> valbuf;
  int64_t levels_in_buf = 0;
  int64_t level_idx = 0;
  int64_t values_in_buf = 0;
  int64_t value_idx = 0;

  Int64Cursor() = default;

  Int64Cursor(shared_ptr<parquet::ColumnReader> col, const parquet::ColumnDescriptor* descr)
  :
    holder(move(col))
  {
    r = dynamic_cast<parquet::Int64Reader*>(holder.get());
    if (!r)
    {
      throw runtime_error("Column is not INT64");
    }

    max_def = descr->max_definition_level();
    max_rep = descr->max_repetition_level();

    if (max_def)
    {
      defbuf.resize(BATCH);
    }
    if (max_rep)
    {
      repbuf.resize(BATCH);
    }

    valbuf.resize(BATCH);
  }

  bool refill()
  {
    if (eof)
    {
      return false;
    }

    int64_t values_read = 0;
    levels_in_buf = r->ReadBatch(
        BATCH,
        max_def ? defbuf.data() : nullptr,
        max_rep ? repbuf.data() : nullptr,
        valbuf.data(),
        &values_read);

    if (levels_in_buf == 0)
    {
      eof = true;
      return false;
    }

    level_idx = 0;
    value_idx = 0;
    values_in_buf = values_read;
    return true;
  }

  bool ensure_pending()
  {
    if (eof && !has_pending)
    {
      return false;
    }

    if (has_pending)
    {
      return true;
    }

    if (level_idx >= levels_in_buf)
    {
      if (!refill())
      {
        return false;
      }
    }

    Entry e{};
    e.valid = true;

    e.def = max_def ? defbuf[level_idx] : 0;
    e.rep = max_rep ? repbuf[level_idx] : 0;

    if (e.def == max_def)
    {
      if (value_idx >= values_in_buf)
      {
        throw runtime_error("value_idx overflow");
      }
      e.has_value = true;
      e.value = valbuf[value_idx++];
    }
    else
    {
      e.has_value = false;
      e.value = 0;
    }

    ++level_idx;

    pending = e;
    has_pending = true;
    return true;
  }

  Entry take()
  {
    if (!ensure_pending())
    {
      return Entry{}; // invalid
    }

    has_pending = false;
    return pending;
  }

  const Entry* peek()
  {
    if (!ensure_pending())
    {
      return nullptr;
    }

    return &pending;
  }
};

static int find_col_idx(const parquet::SchemaDescriptor* schema, const string& name)
{
  for (int i = 0; i < schema->num_columns(); ++i)
  {
    if (schema->Column(i)->path()->ToDotString() == name)
    {
      return i;
    }
  }
  return -1;
}

// Read LIST from a single leaf (for px-only or qty-only). Returns count appended.
static uint32_t append_list_from_leaf_for_row(
    Int64Cursor& leaf,
    vector<int64_t>* out_vals)
{
  const Entry* p = leaf.peek();
  if (!p)
  {
    return 0;
  }

  bool empty_start = (!p->has_value && p->rep == 0);
  if (empty_start)
  {
    (void)leaf.take();
    return 0;
  }

  uint32_t count = 0;
  while (true)
  {
    Entry e = leaf.take();
    if (out_vals)
    {
      out_vals->push_back(e.value);
    }
    ++count;

    const Entry* n = leaf.peek();
    if (!n || n->rep == 0)
    {
      break;
    }
  }
  return count;
}

// Read LIST pairs (px, qty) per row; returns number of pairs appended.
static uint32_t append_list_pairs_for_row_typed(
    Int64Cursor& px, Int64Cursor& qty,
    vector<int64_t>* out_px,
    vector<int64_t>* out_qty)
{
  const Entry* p_peek = px.peek();
  const Entry* q_peek = qty.peek();

  if (!p_peek || !q_peek)
  {
    return 0;
  }

  bool empty_start = (!p_peek->has_value && p_peek->rep == 0) &&
                     (!q_peek->has_value && q_peek->rep == 0);

  if (empty_start)
  {
    (void)px.take();
    (void)qty.take();
    return 0;
  }

  uint32_t count = 0;

  while (true)
  {
    Entry epx = px.take();
    Entry eqy = qty.take();

    if (out_px)
    {
      out_px->push_back(epx.value);
    }
    if (out_qty)
    {
      out_qty->push_back(eqy.value);
    }
    ++count;

    const Entry* npx = px.peek();
    const Entry* nqy = qty.peek();
    if (!npx || !nqy)
    {
      break;
    }
    if (npx->rep == 0)
    {
      break;
    }
    if (npx->rep != nqy->rep)
    {
      break; // defensive
    }
  }

  return count;
}

// ======== Date helpers & file mapping (chronological order, no lexicographic sort) ========

struct YMD
{
  int year;
  int month;
  int day;
};

static YMD ymd_utc_from_ns(int64_t ns)
{
  time_t s = static_cast<time_t>(ns / 1'000'000'000LL);
  tm tm{};
  gmtime_r(&s, &tm);
  return {tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday};
}

static int64_t floor_day_ns(int64_t ns)
{
  int64_t sec = ns / 1'000'000'000LL;
  int64_t day_sec = sec - (sec % 86400LL);
  return day_sec * 1'000'000'000LL;
}

static vector<string> candidate_files(const string& root,
                                      const string& symb,
                                      const string& type,
                                      int64_t start_ns,
                                      int64_t end_ns)
{
  vector<string> paths;

  if (start_ns >= end_ns)
  {
    return paths;
  }

  int64_t t = floor_day_ns(start_ns);
  const int64_t end_floor = floor_day_ns(end_ns - 1);

  for (; t <= end_floor; t += 86'400'000'000'000LL)
  {
    auto ymd = ymd_utc_from_ns(t);

    ostringstream p;
    p << root << '/' << symb
      << '/' << ymd.year
      << '/' << ymd.month
      << "/binance_" << type << '_' << ymd.day << ".parquet";

    if (fs::exists(p.str()))
    {
      // Already chronological because we step day by day
      paths.push_back(p.str());
    }
  }

  // No sort/unique: avoid lexicographic "1,10,11,2..."
  return paths;
}

// ======== RowGroup -> column vectors (decode once per RG) ========

struct FileStreamerTopCols
{
  unique_ptr<parquet::ParquetFileReader> reader;
  shared_ptr<parquet::FileMetaData> md;
  const parquet::SchemaDescriptor* schema = nullptr;
  int rg_idx = 0;

  explicit FileStreamerTopCols(string path)
  {
    cout << path << endl;
    reader = parquet::ParquetFileReader::OpenFile(path, /*memory_map=*/false);
    md     = reader->metadata();
    schema = md->schema();
  }

  bool next_rg(
      int64_t start_ns, int64_t end_ns, const TopSelect& sel,
      vector<int64_t>& v_ts, vector<int64_t>& v_apx, vector<int64_t>& v_aq,
      vector<int64_t>& v_bpx, vector<int64_t>& v_bq, vector<int64_t>& v_val)
  {
    while (true)
    {
      if (rg_idx >= md->num_row_groups())
      {
        return false;
      }

      shared_ptr<parquet::RowGroupReader> rg = reader->RowGroup(rg_idx++);
      auto rmd = rg->metadata();
      int64_t rows = rmd->num_rows();

      int ts_i  = find_col_idx(schema, "ts");
      if (ts_i < 0)
      {
        throw runtime_error("top: missing ts");
      }

      Int64Cursor ts (rg->Column(ts_i), schema->Column(ts_i));

      optional<Int64Cursor> apx, aq, bpx, bq, val;

      if (sel.askPx)
      {
        int apx_i = find_col_idx(schema, "askPx");
        if (apx_i < 0) { throw runtime_error("top: missing askPx"); }
        apx.emplace(rg->Column(apx_i), schema->Column(apx_i));
      }
      if (sel.askQty)
      {
        int aq_i = find_col_idx(schema, "askQty");
        if (aq_i < 0) { throw runtime_error("top: missing askQty"); }
        aq.emplace(rg->Column(aq_i), schema->Column(aq_i));
      }
      if (sel.bidPx)
      {
        int bpx_i = find_col_idx(schema, "bidPx");
        if (bpx_i < 0) { throw runtime_error("top: missing bidPx"); }
        bpx.emplace(rg->Column(bpx_i), schema->Column(bpx_i));
      }
      if (sel.bidQty)
      {
        int bq_i = find_col_idx(schema, "bidQty");
        if (bq_i < 0) { throw runtime_error("top: missing bidQty"); }
        bq.emplace(rg->Column(bq_i), schema->Column(bq_i));
      }
      if (sel.valu)
      {
        int val_i = find_col_idx(schema, "valu");
        if (val_i < 0) { throw runtime_error("top: missing valu"); }
        val.emplace(rg->Column(val_i), schema->Column(val_i));
      }

      v_ts.clear(); v_apx.clear(); v_aq.clear();
      v_bpx.clear(); v_bq.clear(); v_val.clear();

      v_ts.reserve(rows);
      if (sel.askPx)  { v_apx.reserve(rows); }
      if (sel.askQty) { v_aq.reserve(rows); }
      if (sel.bidPx)  { v_bpx.reserve(rows); }
      if (sel.bidQty) { v_bq.reserve(rows); }
      if (sel.valu)   { v_val.reserve(rows); }

      for (int64_t r = 0; r < rows; ++r)
      {
        Entry e_ts = ts.take();

        Entry e_apx{}, e_aq{}, e_bpx{}, e_bq{}, e_val{};

        if (sel.askPx)  { e_apx  = apx->take(); }
        if (sel.askQty) { e_aq   = aq->take(); }
        if (sel.bidPx)  { e_bpx  = bpx->take(); }
        if (sel.bidQty) { e_bq   = bq->take(); }
        if (sel.valu)   { e_val  = val->take(); }

        if (e_ts.value >= start_ns && e_ts.value < end_ns)
        {
          v_ts.push_back(e_ts.value);
          if (sel.askPx)  { v_apx.push_back(e_apx.value); }
          if (sel.askQty) { v_aq.push_back(e_aq.value); }
          if (sel.bidPx)  { v_bpx.push_back(e_bpx.value); }
          if (sel.bidQty) { v_bq.push_back(e_bq.value); }
          if (sel.valu)   { v_val.push_back(e_val.value); }
        }
      }

      if (!v_ts.empty())
      {
        return true;
      }
    }
  }
};

struct FileStreamerDeltaCols
{
  unique_ptr<parquet::ParquetFileReader> reader;
  shared_ptr<parquet::FileMetaData> md;
  const parquet::SchemaDescriptor* schema = nullptr;
  int rg_idx = 0;

  explicit FileStreamerDeltaCols(string path)
  {
    cout << path << endl;
    reader = parquet::ParquetFileReader::OpenFile(path, /*memory_map=*/false);
    md     = reader->metadata();
    schema = md->schema();
  }

  bool next_rg(
      int64_t start_ns, int64_t end_ns, const DeltaSelect& sel,
      vector<int64_t>& v_ts, vector<int64_t>& v_fid, vector<int64_t>& v_lid, vector<int64_t>& v_evt,
      vector<uint32_t>& ask_off, vector<int64_t>& ask_px, vector<int64_t>& ask_qty,
      vector<uint32_t>& bid_off, vector<int64_t>& bid_px, vector<int64_t>& bid_qty)
  {
    while (true)
    {
      if (rg_idx >= md->num_row_groups())
      {
        return false;
      }

      shared_ptr<parquet::RowGroupReader> rg = reader->RowGroup(rg_idx++);
      auto rmd = rg->metadata();
      int64_t rows = rmd->num_rows();

      int ts_i = find_col_idx(schema, "ts");
      if (ts_i < 0)
      {
        throw runtime_error("delta: missing ts");
      }
      Int64Cursor ts(rg->Column(ts_i), schema->Column(ts_i));

      optional<Int64Cursor> fid, lid, evt;

      if (sel.firstId)
      {
        int fid_i = find_col_idx(schema, "firstId");
        if (fid_i < 0) { throw runtime_error("delta: missing firstId"); }
        fid.emplace(rg->Column(fid_i), schema->Column(fid_i));
      }
      if (sel.lastId)
      {
        int lid_i = find_col_idx(schema, "lastId");
        if (lid_i < 0) { throw runtime_error("delta: missing lastId"); }
        lid.emplace(rg->Column(lid_i), schema->Column(lid_i));
      }
      if (sel.eventTime)
      {
        int evt_i = find_col_idx(schema, "eventTime");
        if (evt_i < 0) { throw runtime_error("delta: missing eventTime"); }
        evt.emplace(rg->Column(evt_i), schema->Column(evt_i));
      }

      bool need_asks = (sel.ask_px || sel.ask_qty);
      bool need_bids = (sel.bid_px || sel.bid_qty);

      optional<Int64Cursor> apx, aqty, bpx, bqty;

      if (need_asks)
      {
        if (sel.ask_px)
        {
          int apx_i = find_col_idx(schema, "ask.list.element.px");
          if (apx_i < 0) { throw runtime_error("delta: missing ask px"); }
          apx.emplace(rg->Column(apx_i), schema->Column(apx_i));
        }
        if (sel.ask_qty)
        {
          int aqty_i = find_col_idx(schema, "ask.list.element.qty");
          if (aqty_i < 0) { throw runtime_error("delta: missing ask qty"); }
          aqty.emplace(rg->Column(aqty_i), schema->Column(aqty_i));
        }
      }

      if (need_bids)
      {
        if (sel.bid_px)
        {
          int bpx_i = find_col_idx(schema, "bid.list.element.px");
          if (bpx_i < 0) { throw runtime_error("delta: missing bid px"); }
          bpx.emplace(rg->Column(bpx_i), schema->Column(bpx_i));
        }
        if (sel.bid_qty)
        {
          int bqty_i = find_col_idx(schema, "bid.list.element.qty");
          if (bqty_i < 0) { throw runtime_error("delta: missing bid qty"); }
          bqty.emplace(rg->Column(bqty_i), schema->Column(bqty_i));
        }
      }

      v_ts.clear(); v_fid.clear(); v_lid.clear(); v_evt.clear();
      ask_off.clear(); ask_px.clear(); ask_qty.clear();
      bid_off.clear(); bid_px.clear(); bid_qty.clear();

      v_ts.reserve(rows);
      if (sel.firstId)   { v_fid.reserve(rows); }
      if (sel.lastId)    { v_lid.reserve(rows); }
      if (sel.eventTime) { v_evt.reserve(rows); }
      if (need_asks)     { ask_off.reserve(rows + 1); ask_off.push_back(0); }
      if (need_bids)     { bid_off.reserve(rows + 1); bid_off.push_back(0); }

      for (int64_t r = 0; r < rows; ++r)
      {
        Entry e_ts = ts.take();

        Entry e_fid{}, e_lid{}, e_evt{};
        if (sel.firstId)   { e_fid = fid->take(); }
        if (sel.lastId)    { e_lid = lid->take(); }
        if (sel.eventTime) { e_evt = evt->take(); }

        bool in_range = (e_ts.value >= start_ns && e_ts.value < end_ns);

        uint32_t asks_added = 0, bids_added = 0;

        if (need_asks)
        {
          if (sel.ask_px && sel.ask_qty)
          {
            asks_added = append_list_pairs_for_row_typed(
                *apx, *aqty,
                in_range ? &ask_px : nullptr,
                in_range ? &ask_qty : nullptr);
          }
          else if (sel.ask_px)
          {
            asks_added = append_list_from_leaf_for_row(
                *apx,
                in_range ? &ask_px : nullptr);
          }
          else if (sel.ask_qty)
          {
            asks_added = append_list_from_leaf_for_row(
                *aqty,
                in_range ? &ask_qty : nullptr);
          }
        }

        if (need_bids)
        {
          if (sel.bid_px && sel.bid_qty)
          {
            bids_added = append_list_pairs_for_row_typed(
                *bpx, *bqty,
                in_range ? &bid_px : nullptr,
                in_range ? &bid_qty : nullptr);
          }
          else if (sel.bid_px)
          {
            bids_added = append_list_from_leaf_for_row(
                *bpx,
                in_range ? &bid_px : nullptr);
          }
          else if (sel.bid_qty)
          {
            bids_added = append_list_from_leaf_for_row(
                *bqty,
                in_range ? &bid_qty : nullptr);
          }
        }

        if (in_range)
        {
          v_ts.push_back(e_ts.value);
          if (sel.firstId)   { v_fid.push_back(e_fid.value); }
          if (sel.lastId)    { v_lid.push_back(e_lid.value); }
          if (sel.eventTime) { v_evt.push_back(e_evt.value); }

          if (need_asks) { ask_off.push_back(ask_off.back() + asks_added); }
          if (need_bids) { bid_off.push_back(bid_off.back() + bids_added); }
        }
      }

      if (!v_ts.empty())
      {
        return true;
      }
    }
  }
};

// ======== Public DB + columnar-batch readers ========

class ShardedDB
{
public:
  explicit ShardedDB(string root)
  :
    root_(move(root))
  {}

  struct TopBatchReader
  {
    TopBatchReader(vector<string> files, int64_t s, int64_t e, TopSelect sel)
    :
      files_(move(files)), start_ns_(s), end_ns_(e), sel_(sel)
    {}

    bool next(TopColsView& out)
    {
      while (true)
      {
        if (!fs_)
        {
          if (file_idx_ >= files_.size())
          {
            return false;
          }

          try
          {
            fs_ = make_unique<FileStreamerTopCols>(files_[file_idx_]);
          }
          catch (const exception& e)
          {
            cerr << "WARN: open failed: " << files_[file_idx_] << " : " << e.what() << "\n";
            ++file_idx_;
            continue;
          }
        }

        bool ok = false;

        try
        {
          ok = fs_->next_rg(start_ns_, end_ns_, sel_, ts_, apx_, aq_, bpx_, bq_, val_);
        }
        catch (const exception& e)
        {
          cerr << "WARN: read failed: " << files_[file_idx_] << " : " << e.what() << "\n";
          ok = false;
        }

        if (!ok)
        {
          fs_.reset();
          ++file_idx_;
          continue;
        }

        if (ts_.empty())
        {
          continue;
        }

        out.ts     = sel_.ts     ? ts_.data()  : nullptr;
        out.askPx  = sel_.askPx  ? apx_.data() : nullptr;
        out.askQty = sel_.askQty ? aq_.data()  : nullptr;
        out.bidPx  = sel_.bidPx  ? bpx_.data() : nullptr;
        out.bidQty = sel_.bidQty ? bq_.data()  : nullptr;
        out.valu   = sel_.valu   ? val_.data() : nullptr;
        out.n      = ts_.size();
        return true;
      }
    }

  private:
    vector<string> files_;
    size_t file_idx_ = 0;
    unique_ptr<FileStreamerTopCols> fs_;
    int64_t start_ns_, end_ns_;
    TopSelect sel_;

    vector<int64_t> ts_, apx_, aq_, bpx_, bq_, val_;
  };

  struct DeltaBatchReader
  {
    DeltaBatchReader(vector<string> files, int64_t s, int64_t e, DeltaSelect sel)
    :
      files_(move(files)), start_ns_(s), end_ns_(e), sel_(sel)
    {}

    bool next(DeltaColsView& out)
    {
      while (true)
      {
        if (!fs_)
        {
          if (file_idx_ >= files_.size())
          {
            return false;
          }

          try
          {
            fs_ = make_unique<FileStreamerDeltaCols>(files_[file_idx_]);
          }
          catch (const exception& e)
          {
            cerr << "WARN: open failed: " << files_[file_idx_] << " : " << e.what() << "\n";
            ++file_idx_;
            continue;
          }
        }

        bool ok = false;

        try
        {
          ok = fs_->next_rg(
              start_ns_, end_ns_, sel_,
              ts_, fid_, lid_, evt_,
              ask_off_, ask_px_, ask_qty_,
              bid_off_, bid_px_, bid_qty_);
        }
        catch (const exception& e)
        {
          cerr << "WARN: read failed: " << files_[file_idx_] << " : " << e.what() << "\n";
          ok = false;
        }

        if (!ok)
        {
          fs_.reset();
          ++file_idx_;
          continue;
        }

        if (ts_.empty())
        {
          continue;
        }

        out.ts        = sel_.ts        ? ts_.data()  : nullptr;
        out.firstId   = sel_.firstId   ? fid_.data() : nullptr;
        out.lastId    = sel_.lastId    ? lid_.data() : nullptr;
        out.eventTime = sel_.eventTime ? evt_.data() : nullptr;
        out.n         = ts_.size();

        bool have_asks = sel_.ask_px || sel_.ask_qty;
        bool have_bids = sel_.bid_px || sel_.bid_qty;

        out.ask_off = have_asks ? ask_off_.data() : nullptr;
        out.ask_px  = sel_.ask_px ? ask_px_.data() : nullptr;
        out.ask_qty = sel_.ask_qty ? ask_qty_.data() : nullptr;

        out.bid_off = have_bids ? bid_off_.data() : nullptr;
        out.bid_px  = sel_.bid_px ? bid_px_.data() : nullptr;
        out.bid_qty = sel_.bid_qty ? bid_qty_.data() : nullptr;

        return true;
      }
    }

  private:
    vector<string> files_;
    size_t file_idx_ = 0;
    unique_ptr<FileStreamerDeltaCols> fs_;
    int64_t start_ns_, end_ns_;
    DeltaSelect sel_;

    vector<int64_t> ts_, fid_, lid_, evt_;
    vector<uint32_t> ask_off_, bid_off_;
    vector<int64_t>  ask_px_,  ask_qty_, bid_px_, bid_qty_;
  };

  unique_ptr<TopBatchReader> get_top_cols(
      int64_t start_ns, int64_t end_ns, const string& symb, TopSelect sel = {}) const
  {
    auto files = candidate_files(root_, symb, "top", start_ns, end_ns);
    return make_unique<TopBatchReader>(move(files), start_ns, end_ns, sel);
  }

  unique_ptr<DeltaBatchReader> get_delta_cols(
      int64_t start_ns, int64_t end_ns, const string& symb, DeltaSelect sel = {}) const
  {
    auto files = candidate_files(root_, symb, "delta", start_ns, end_ns);
    return make_unique<DeltaBatchReader>(move(files), start_ns, end_ns, sel);
  }

private:
  string root_;
};

// ======== Column selection helpers (CLI) ========

static string norm_token(string s)
{
  for (char& c : s)
  {
    c = (char)tolower((unsigned char)c);
  }

  string out;
  out.reserve(s.size());
  for (char c : s)
  {
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
    {
      out.push_back(c);
    }
  }
  return out;
}

static vector<string> split_csv(const string& s)
{
  vector<string> v;
  string cur;
  for (char c : s)
  {
    if (c == ',')
    {
      if (!cur.empty())
      {
        v.push_back(cur);
        cur.clear();
      }
    }
    else
    {
      cur.push_back(c);
    }
  }
  if (!cur.empty())
  {
    v.push_back(cur);
  }
  return v;
}

static TopSelect make_top_select_from_csv(const string& csv)
{
  TopSelect sel{};
  sel.ts = sel.askPx = sel.askQty = sel.bidPx = sel.bidQty = sel.valu = false;

  for (string t : split_csv(csv))
  {
    string k = norm_token(t);
    if (k == "ts" || k == "time") { sel.ts = true; }
    else if (k == "askpx" || k == "px" || k == "ask" || k == "askprice") { sel.askPx = true; }
    else if (k == "askqty" || k == "qty" || k == "asksize") { sel.askQty = true; }
    else if (k == "bidpx" || k == "bid" || k == "bidprice") { sel.bidPx = true; }
    else if (k == "bidqty" || k == "bidsize") { sel.bidQty = true; }
    else if (k == "valu" || k == "value" || k == "vol" || k == "volume") { sel.valu = true; }
  }
  return sel;
}

static DeltaSelect make_delta_select_from_csv(const string& csv)
{
  DeltaSelect sel{};
  sel.ts = sel.firstId = sel.lastId = sel.eventTime = false;
  sel.ask_px = sel.ask_qty = sel.bid_px = sel.bid_qty = false;

  for (string t : split_csv(csv))
  {
    string k = norm_token(t);
    if (k == "ts" || k == "time") { sel.ts = true; }
    else if (k == "firstid" || k == "fid") { sel.firstId = true; }
    else if (k == "lastid"  || k == "lid") { sel.lastId = true; }
    else if (k == "eventtime" || k == "evt" || k == "event") { sel.eventTime = true; }

    else if (k == "askpx" || k == "px" || k == "ask" || k == "askprice") { sel.ask_px = true; }
    else if (k == "askqty" || k == "qty" || k == "asksize") { sel.ask_qty = true; }
    else if (k == "bidpx" || k == "bid" || k == "bidprice") { sel.bid_px = true; }
    else if (k == "bidqty" || k == "bidsize") { sel.bid_qty = true; }
  }
  return sel;
}

// ======== Example CLI (prints every 1,000,000th row) ========
// Usage: ./a.out ROOT SYMB TYPE START_NS END_NS [columns_csv]
//   Examples:
//     top ts+askPx: ./a.out ROOT SYMB top   START_NS END_NS ts,askPx
//     delta ask px: ./a.out ROOT SYMB delta START_NS END_NS ts,firstId,lastId,eventTime,askPx
int main(int argc, char** argv)
{
  if (argc < 6)
  {
    cerr << "Usage: " << argv[0] << " <root> <symb> <type: top|delta> <start_ns> <end_ns> [columns_csv]\n";
    return 1;
  }

  string root = argv[1];
  string symb = argv[2];
  string type = argv[3];
  int64_t start_ns = stoll(argv[4]);
  int64_t end_ns   = stoll(argv[5]);

  ShardedDB db(root);

  if (type == "top")
  {
    TopSelect sel{};
    if (argc >= 7)
    {
      sel = make_top_select_from_csv(argv[6]); // else defaults: all true
    }

    auto rdr = db.get_top_cols(start_ns, end_ns, symb, sel);

    TopColsView v{};
    uint64_t seen = 0;
    while (rdr->next(v))
    {
      for (size_t i = 0; i < v.n; ++i)
      {
        ++seen;
        if (seen % 1000000 != 0)
        {
          continue;
        }

        bool first = true;
        if (v.ts)     { if (!first) { cout << ';'; } cout << v.ts[i];     first = false; }
        if (v.askPx)  { if (!first) { cout << ';'; } cout << v.askPx[i];  first = false; }
        if (v.askQty) { if (!first) { cout << ';'; } cout << v.askQty[i]; first = false; }
        if (v.bidPx)  { if (!first) { cout << ';'; } cout << v.bidPx[i];  first = false; }
        if (v.bidQty) { if (!first) { cout << ';'; } cout << v.bidQty[i]; first = false; }
        if (v.valu)   { if (!first) { cout << ';'; } cout << v.valu[i];   first = false; }
        cout << '\n';
      }
    }
  }
  else if (type == "delta")
  {
    DeltaSelect sel{};
    if (argc >= 7)
    {
      sel = make_delta_select_from_csv(argv[6]); // else defaults: all true
    }

    auto rdr = db.get_delta_cols(start_ns, end_ns, symb, sel);

    DeltaColsView v{};
    uint64_t seen = 0;
    while (rdr->next(v))
    {
      for (size_t i = 0; i < v.n; ++i)
      {
        ++seen;
        if (seen % 1000000 != 0)
        {
          continue;
        }

        bool first = true;
        auto put = [&](int64_t x)
        {
          if (!first) { cout << ';'; }
          cout << x;
          first = false;
        };

        if (v.ts)        { put(v.ts[i]);        } else { put(0); }
        if (v.firstId)   { put(v.firstId[i]);   } else { put(0); }
        if (v.lastId)    { put(v.lastId[i]);    } else { put(0); }
        if (v.eventTime) { put(v.eventTime[i]); } else { put(0); }

        cout << ';';
        if (v.ask_off && (v.ask_px || v.ask_qty))
        {
          uint32_t a0 = v.ask_off[i], a1 = v.ask_off[i + 1];
          for (uint32_t k = a0; k < a1; ++k)
          {
            if (k > a0) { cout << ','; }
            if (v.ask_px)  { cout << v.ask_px[k]; }
            if (v.ask_px && v.ask_qty) { cout << '('; }
            if (v.ask_qty) { cout << v.ask_qty[k]; }
            if (v.ask_px && v.ask_qty) { cout << ')'; }
          }
        }

        cout << ';';
        if (v.bid_off && (v.bid_px || v.bid_qty))
        {
          uint32_t b0 = v.bid_off[i], b1 = v.bid_off[i + 1];
          for (uint32_t k = b0; k < b1; ++k)
          {
            if (k > b0) { cout << ','; }
            if (v.bid_px)  { cout << v.bid_px[k]; }
            if (v.bid_px && v.bid_qty) { cout << '('; }
            if (v.bid_qty) { cout << v.bid_qty[k]; }
            if (v.bid_px && v.bid_qty) { cout << ')'; }
          }
        }

        cout << '\n';
      }
    }
  }
  else
  {
    cerr << "Unsupported type: " << type << "\n";
    return 2;
  }

  return 0;
}

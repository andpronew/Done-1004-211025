// Low-level Parquet sharded stream reader
// g++ -std=gnu++17 -O2 sharded_low_parquet.cpp -lparquet -larrow -lzstd
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

// ======== Shared low-level bits (your style) ========

struct Entry
{
  int16_t def = 0, rep = 0;
  bool has_value = false;   // true iff a value was produced (non-null)
  int64_t value = 0;        // valid only if has_value
  bool valid = false;       // true once something was read
};

struct Int64Cursor
{
  shared_ptr<parquet::ColumnReader> holder; // keep alive
  parquet::Int64Reader* r = nullptr;             // raw
  int16_t max_def = 0, max_rep = 0;
  bool has_pending = false;
  bool eof = false;
  Entry pending;

  Int64Cursor() = default;
  Int64Cursor(shared_ptr<parquet::ColumnReader> col, const parquet::ColumnDescriptor* descr)
  :
    holder(move(col))
  {
    r = dynamic_cast<parquet::Int64Reader*>(holder.get());
    if (!r)
      throw runtime_error("Column is not INT64");
    max_def = descr->max_definition_level();
    max_rep = descr->max_repetition_level();
  }

  bool ensure_pending()
  {
    if (eof)
      return false;
    if (has_pending)
      return true;

    int64_t values_read = 0;
    Entry e;
    int64_t levels = r->ReadBatch(
        /*batch_size=*/1,
        max_def ? &e.def : nullptr,
        max_rep ? &e.rep : nullptr,
        &e.value,
        &values_read);

    if (levels == 0)
    {
      eof = true;
      return false;
    }
    e.has_value = (values_read == 1);
    if (max_rep == 0)
      e.rep = 0; // scalars
    e.valid = true;

    pending = e;
    has_pending = true;
    return true;
  }

  Entry take()
  {
    if (!ensure_pending())
      return Entry{}; // invalid
    has_pending = false;
    return pending;
  }

  const Entry* peek()
  {
    if (!ensure_pending())
      return nullptr;
    return &pending;
  }
};

static int find_col_idx(const parquet::SchemaDescriptor* schema, const string& name)
{
  for (int i = 0; i < schema->num_columns(); ++i)
  {
    if (schema->Column(i)->path()->ToDotString() == name)
      return i;
  }
  return -1;
}

static inline string print_scalar(const Entry& e)
{
  return e.has_value ? to_string(e.value) : "NULL";
}

// Read all list elements (as pairs) for the current top-level row from two leaf cursors.
// Assumes both columns come from the same LIST<STRUCT<px:int64, qty:int64>>.
static void read_list_pairs_for_row(Int64Cursor& px, Int64Cursor& qty, vector<pair<bool,int64_t>>& out_px, vector<pair<bool,int64_t>>& out_qty)
{
  out_px.clear();
  out_qty.clear();

  const Entry* p_peek = px.peek();
  const Entry* q_peek = qty.peek();
  if (!p_peek || !q_peek)
    return;

  // Empty list sentinel at start of row: rep==0 and no value on both leaves
  bool empty_start = (!p_peek->has_value && p_peek->rep == 0) && (!q_peek->has_value && q_peek->rep == 0);

  if (empty_start)
  {
    (void)px.take();
    (void)qty.take();
    return;
  }

  // Otherwise consume elements until we see rep==0 (next top-level row)
  while (true)
  {
    Entry epx = px.take();
    Entry eqy = qty.take();

    out_px.emplace_back(epx.has_value, epx.value);
    out_qty.emplace_back(eqy.has_value, eqy.value);

    const Entry* npx = px.peek();
    const Entry* nqy = qty.peek();
    if (!npx || !nqy)
      break;
    if (npx->rep == 0)
      break;
    if (npx->rep != nqy->rep)
      break; // defensive
  }
}

// ======== Date helpers & file mapping ========

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

static vector<string> candidate_files(const string& root, const string& symb, const string& type, int64_t start_ns, int64_t end_ns)
{
  vector<string> paths;
  if (start_ns >= end_ns)
    return paths;

  int64_t t = floor_day_ns(start_ns);
  const int64_t end_floor = floor_day_ns(end_ns - 1); // inclusive last day if range crosses day
  for (; t <= end_floor; t += 86'400'000'000'000LL)
  {
    auto ymd = ymd_utc_from_ns(t);
    ostringstream p;
    p << root << '/' << symb << '/' << ymd.year << '/' << ymd.month << "/binance_" << type << '_' << ymd.day << ".parquet";
    if (fs::exists(p.str()))
      paths.push_back(p.str());
  }

  // de-dup + sort for stable order
  sort(paths.begin(), paths.end());
  paths.erase(unique(paths.begin(), paths.end()), paths.end());

  return paths;
}

// ======== RowGroup emitters ========

struct IRowGroupEmitter
{
  virtual ~IRowGroupEmitter() = default;
  virtual bool next_line_in_range(int64_t start_ns, int64_t end_ns, string& out) = 0;
  virtual bool exhausted() const = 0;
};

// ---- TOP (flat) ---- ts;askPx(askQty);bidPx(bidQty);valu
struct TopRowGroupEmitter : IRowGroupEmitter
{
  int64_t rows = 0, r = 0;

  Int64Cursor ts, apx, aq, bpx, bq, val;

  TopRowGroupEmitter(shared_ptr<parquet::RowGroupReader> rg, const parquet::SchemaDescriptor* schema) : rg_(move(rg))
  {
    int ts_i   = find_col_idx(schema, "ts");
    if (ts_i  < 0)
      throw runtime_error("missing ts");
    int apx_i  = find_col_idx(schema, "askPx");
    if (apx_i < 0)
      throw runtime_error("missing askPx");
    int aq_i   = find_col_idx(schema, "askQty");
    if (aq_i  < 0)
      throw runtime_error("missing askQty");
    int bpx_i  = find_col_idx(schema, "bidPx");
    if (bpx_i < 0)
      throw runtime_error("missing bidPx");
    int bq_i   = find_col_idx(schema, "bidQty");
    if (bq_i  < 0)
      throw runtime_error("missing bidQty");
    int val_i  = find_col_idx(schema, "valu");
    if (val_i < 0)
      throw runtime_error("missing valu");

    auto md = rg_->metadata();
    rows = md->num_rows();

    ts  = Int64Cursor(rg_->Column(ts_i),  schema->Column(ts_i));
    apx = Int64Cursor(rg_->Column(apx_i), schema->Column(apx_i));
    aq  = Int64Cursor(rg_->Column(aq_i),  schema->Column(aq_i));
    bpx = Int64Cursor(rg_->Column(bpx_i), schema->Column(bpx_i));
    bq  = Int64Cursor(rg_->Column(bq_i),  schema->Column(bq_i));
    val = Int64Cursor(rg_->Column(val_i), schema->Column(val_i));
  }

  bool next_line_in_range(int64_t start_ns, int64_t end_ns, string& out) override
  {
    while (r < rows)
    {
      Entry e_ts  = ts.take();
      Entry e_apx = apx.take();
      Entry e_aq  = aq.take();
      Entry e_bpx = bpx.take();
      Entry e_bq  = bq.take();
      Entry e_val = val.take();
      ++r;

      bool in_range = (e_ts.has_value && e_ts.value >= start_ns && e_ts.value < end_ns);
      if (!in_range)
        continue;

      ostringstream line;
      line << print_scalar(e_ts) << ';'
           << print_scalar(e_apx) << '(' << print_scalar(e_aq)  << ')' << ';'
           << print_scalar(e_bpx) << '(' << print_scalar(e_bq)  << ')' << ';'
           << print_scalar(e_val) << '\n';
      out = move(line).str();
      return true;
    }
    return false;
  }

  bool exhausted() const override
  {
    return r >= rows;
  }

private:
  shared_ptr<parquet::RowGroupReader> rg_;
};

// ---- DELTA (nested LIST<STRUCT>) ----
// ts;firstId;lastId;eventTime;px(qty),px(qty)...;px(qty),...
struct DeltaRowGroupEmitter : IRowGroupEmitter
{
  int64_t rows = 0, r = 0;

  Int64Cursor ts, fid, lid, evt;
  Int64Cursor apx, aqty, bpx, bqty;
  vector<pair<bool,int64_t>> ask_px, ask_qty, bid_px, bid_qty;

  DeltaRowGroupEmitter(shared_ptr<parquet::RowGroupReader> rg, const parquet::SchemaDescriptor* schema) : rg_(move(rg))
  {
    auto md = rg_->metadata();
    rows = md->num_rows();

    const char* TS   = "ts";
    const char* FID  = "firstId";
    const char* LID  = "lastId";
    const char* EVT  = "eventTime";
    const char* APX  = "ask.list.element.px";
    const char* AQTY = "ask.list.element.qty";
    const char* BPX  = "bid.list.element.px";
    const char* BQTY = "bid.list.element.qty";

    int ts_i   = find_col_idx(schema, TS);
    int fid_i  = find_col_idx(schema, FID);
    int lid_i  = find_col_idx(schema, LID);
    int evt_i  = find_col_idx(schema, EVT);
    int apx_i  = find_col_idx(schema, APX);
    int aqty_i = find_col_idx(schema, AQTY);
    int bpx_i  = find_col_idx(schema, BPX);
    int bqty_i = find_col_idx(schema, BQTY);

    if (ts_i<0||fid_i<0||lid_i<0||evt_i<0||apx_i<0||aqty_i<0||bpx_i<0||bqty_i<0)
      throw runtime_error("delta: required columns missing");

    ts   = Int64Cursor(rg_->Column(ts_i),   schema->Column(ts_i));
    fid  = Int64Cursor(rg_->Column(fid_i),  schema->Column(fid_i));
    lid  = Int64Cursor(rg_->Column(lid_i),  schema->Column(lid_i));
    evt  = Int64Cursor(rg_->Column(evt_i),  schema->Column(evt_i));
    apx  = Int64Cursor(rg_->Column(apx_i),  schema->Column(apx_i));
    aqty = Int64Cursor(rg_->Column(aqty_i), schema->Column(aqty_i));
    bpx  = Int64Cursor(rg_->Column(bpx_i),  schema->Column(bpx_i));
    bqty = Int64Cursor(rg_->Column(bqty_i), schema->Column(bqty_i));
  }

  bool next_line_in_range(int64_t start_ns, int64_t end_ns, string& out) override
  {
    while (r < rows)
    {
      Entry e_ts  = ts.take();
      Entry e_fid = fid.take();
      Entry e_lid = lid.take();
      Entry e_evt = evt.take();

      read_list_pairs_for_row(apx, aqty, ask_px, ask_qty);
      read_list_pairs_for_row(bpx, bqty, bid_px, bid_qty);
      ++r;

      bool in_range = (e_ts.has_value && e_ts.value >= start_ns && e_ts.value < end_ns);
      if (!in_range)
        continue;

      ostringstream line;
      line << print_scalar(e_ts)  << ';'
           << print_scalar(e_fid) << ';'
           << print_scalar(e_lid) << ';'
           << print_scalar(e_evt) << ';';

      // asks
      for (size_t i = 0; i < ask_px.size(); ++i)
      {
        if (i)
          line << ',';
        const auto& pxv  = ask_px[i];
        const auto& qtyv = ask_qty[i];
        line << (pxv.first ? to_string(pxv.second) : "NULL")
             << '('
             << (qtyv.first ? to_string(qtyv.second) : "NULL")
             << ')';
      }
      line << ';';
      // bids
      for (size_t i = 0; i < bid_px.size(); ++i)
      {
        if (i)
          line << ',';
        const auto& pxv  = bid_px[i];
        const auto& qtyv = bid_qty[i];
        line << (pxv.first ? to_string(pxv.second) : "NULL")
             << '('
             << (qtyv.first ? to_string(qtyv.second) : "NULL")
             << ')';
      }
      line << '\n';

      out = move(line).str();
      return true;
    }
    return false;
  }

  bool exhausted() const override
  {
    return r >= rows;
  }

private:
  shared_ptr<parquet::RowGroupReader> rg_;
};

// ======== File streamer ========

struct FileStreamer
{
  unique_ptr<parquet::ParquetFileReader> reader;
  shared_ptr<parquet::FileMetaData> file_md;
  const parquet::SchemaDescriptor* schema = nullptr;
  unique_ptr<IRowGroupEmitter> current_rg;
  int rg_idx = 0;

  string type;

  FileStreamer(string path, string t) : type(move(t))
  {
    reader = parquet::ParquetFileReader::OpenFile(path, /*memory_map=*/false);
    file_md = reader->metadata();
    schema  = file_md->schema();
  }

  bool next_line(int64_t start_ns, int64_t end_ns, string& out)
  {
    while (true)
    {
      if (!current_rg)
      {
        if (rg_idx >= file_md->num_row_groups())
          return false;
        shared_ptr<parquet::RowGroupReader> rg_reader = reader->RowGroup(rg_idx++);
        if (type == "top")
        {
          current_rg = make_unique<TopRowGroupEmitter>(move(rg_reader), schema);
        }
        else if (type == "delta")
        {
          current_rg = make_unique<DeltaRowGroupEmitter>(move(rg_reader), schema);
        }
        else
        {
          throw runtime_error("unsupported type: " + type + " (implement a reader)");
        }
      }

      string line;
      if (current_rg->next_line_in_range(start_ns, end_ns, line))
      {
        out = move(line);
        return true;
      }

      if (current_rg->exhausted())
      {
        current_rg.reset();
        continue;
      }

      // Shouldn't get here, but break to avoid infinite loops.
      return false;
    }
  }
};

// ======== Public DB + Reader ========

class ShardedDB
{
public:
  explicit ShardedDB(string root)
  :
    root_(move(root))
  {}

  struct Reader
  {
    // returns bytes written into buf; 0 => EOF
    size_t next(void* buf, size_t buf_sz)
    {
      char* out = static_cast<char*>(buf);
      size_t wrote = 0;

      // write any pending remainder first
      if (!pending_.empty())
      {
        size_t take = min(buf_sz, pending_.size() - pending_off_);
        memcpy(out, pending_.data() + pending_off_, take);
        pending_off_ += take;
        wrote += take;
        if (pending_off_ < pending_.size())
          return wrote; // still pending
        pending_.clear();
        pending_off_ = 0;
      }

      while (wrote < buf_sz)
      {
        if (!file_streamer_)
        {
          if (file_idx_ >= files_.size())
            return wrote; // EOF
          try
          {
            file_streamer_ = make_unique<FileStreamer>(files_[file_idx_], type_);
          }
          catch (const exception& e)
          {
            // Skip unreadable file but report? For now, skip with stderr.
            cerr << "WARN: open failed: " << files_[file_idx_] << " : " << e.what() << "\n";
            ++file_idx_;
            continue;
          }
        }

        string line;
        bool ok = false;
        try
        {
          ok = file_streamer_->next_line(start_ns_, end_ns_, line);
        }
        catch (const exception& e)
        {
          cerr << "WARN: read failed: " << files_[file_idx_] << " : " << e.what() << "\n";
          ok = false;
        }

        if (!ok)
        {
          // file done; advance
          file_streamer_.reset();
          ++file_idx_;
          continue;
        }

        // emit this line (maybe partially)
        size_t space = buf_sz - wrote;
        if (line.size() <= space)
        {
          memcpy(out + wrote, line.data(), line.size());
          wrote += line.size();
        }
        else
        {
          // spill into pending for the next call
          memcpy(out + wrote, line.data(), space);
          pending_ = move(line);
          pending_off_ = space;
          wrote += space;
          break;
        }
      }
      return wrote;
    }

    // internal
    Reader(vector<string> files, string type, int64_t s, int64_t e)
    :
      files_(move(files)), type_(move(type)), start_ns_(s), end_ns_(e)
    {}

  private:
    vector<string> files_;
    string type_;
    int64_t start_ns_, end_ns_;
    size_t file_idx_ = 0;
    unique_ptr<FileStreamer> file_streamer_;

    string pending_;
    size_t pending_off_ = 0;
  };

  // Build a streaming reader over the matching shards.
  unique_ptr<Reader> get(int64_t start_ns, int64_t end_ns, const string& type, const string& symb) const
  {
    auto files = candidate_files(root_, symb, type, start_ns, end_ns);
    return make_unique<Reader>(move(files), type, start_ns, end_ns);
  }

private:
  string root_;
};

// ======== Example CLI (optional) ========
// Usage: ./a.out ROOT SYMB TYPE START_NS END_NS
// Prints lines to stdout using a 1MB buffer.
int main(int argc, char** argv)
{
  if (argc < 6)
  {
    cerr << "Usage: " << argv[0] << " <root> <symb> <type: top|delta> <start_ns> <end_ns>\n";
    return 1;
  }
  string root = argv[1];
  string symb = argv[2];
  string type = argv[3];
  int64_t start_ns = stoll(argv[4]);
  int64_t end_ns   = stoll(argv[5]);

  ShardedDB db(root);
  auto rdr = db.get(start_ns, end_ns, type, symb);

  vector<char> buf(1 << 20);
  for (;;)
  {
    size_t n = rdr->next(buf.data(), buf.size());
    if (n == 0)
      break;
    cout.write(buf.data(), n);
  }

  return 0;
}


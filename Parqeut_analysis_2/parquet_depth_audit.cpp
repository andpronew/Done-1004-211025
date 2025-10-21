// parquet_bulk_audit_depth.cpp
// Build:
//   g++ -std=gnu++23 -O3 parquet_bulk_audit_depth.cpp -lparquet -larrow -lzstd -o parquet_bulk_audit_depth
//
// Usage:
//   ./parquet_bulk_audit_depth /path/to/parquets anomalies_depth.ndjson
//   ./parquet_bulk_audit_depth /path/to/parquets anomalies_depth_all.ndjson --all   # to write all files
//
// Notes:
// - Focus on depth-style parquet containing columns:
//    ts, firstId, lastId, eventTime, bid.list.element.px, bid.list.element.qty,
//    ask.list.element.px, ask.list.element.qty
// - If bid/ask are flattened (repeated physical values) without offsets, we will compute global stats
//   but won't be able to map per-row arrays to rows. The tool will flag that as informational anomaly.

#include <parquet/api/reader.h>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <limits>
#include <cstdint>
#include <cmath>
#include <iomanip>

using namespace std;
namespace fs = std::filesystem;

static int find_col_idx(const parquet::SchemaDescriptor* schema, const string& name)
{
    for (int i = 0; i < schema->num_columns(); ++i)
        if (schema->Column(i)->path()->ToDotString() == name) return i;
    return -1;
}

static void read_i64_column(parquet::RowGroupReader& rg, int col_idx, vector<int64_t>& out)
{
    const int64_t rows = rg.metadata()->num_rows();
    out.clear();
    out.resize(rows);
    auto col = rg.Column(col_idx);
    auto* r = static_cast<parquet::Int64Reader*>(col.get());
    int64_t done = 0;
    while (done < rows) {
        int64_t values_read = 0;
        int64_t levels = r->ReadBatch(rows - done, nullptr, nullptr, out.data() + done, &values_read);
        if (levels == 0 && values_read == 0) break;
        done += values_read;
    }
    if (done != rows) out.resize(done);
}

// Accumulators
struct Welford {
    long double mean = 0.0L;
    long double m2 = 0.0L;
    uint64_t n = 0;
    void add(long double x) {
        ++n;
        long double delta = x - mean;
        mean += delta / (long double)n;
        long double delta2 = x - mean;
        m2 += delta * delta2;
    }
    long double variance() const { return (n > 1) ? (m2 / (long double)(n - 1)) : 0.0L; }
    long double stddev() const { return sqrt((double)variance()); }
};

struct FileMetric {
    string path;
    int64_t meta_rows = 0;
    int64_t rows_scanned = 0;
    int row_groups = 0;

    // ts
    bool has_ts = false;
    int64_t ts_min = numeric_limits<int64_t>::max();
    int64_t ts_max = numeric_limits<int64_t>::min();
    uint64_t max_gap_ns = 0;
    uint64_t gaps_gt_100ms = 0;
    uint64_t gaps_gt_1s = 0;
    uint64_t non_monotonic_ts = 0;

    // id ranges
    bool has_firstId = false;
    bool has_lastId = false;
    uint64_t prev_lastId = 0;
    uint64_t id_overlap_count = 0;
    uint64_t id_gap_count = 0;
    uint64_t last_lt_first_count = 0;

    // eventTime present?
    bool has_eventTime = false;

    // bid/ask flattened stats (global)
    bool has_bid_px = false;
    uint64_t bid_px_count = 0;
    int64_t bid_px_min = numeric_limits<int64_t>::max();
    int64_t bid_px_max = numeric_limits<int64_t>::min();
    long double bid_px_avg = 0.0L;
    uint64_t bid_px_zero = 0;

    bool has_bid_qty = false;
    uint64_t bid_qty_count = 0;
    int64_t bid_qty_min = numeric_limits<int64_t>::max();
    int64_t bid_qty_max = numeric_limits<int64_t>::min();
    long double bid_qty_avg = 0.0L;
    uint64_t bid_qty_zero = 0;

    bool has_ask_px = false;
    uint64_t ask_px_count = 0;
    int64_t ask_px_min = numeric_limits<int64_t>::max();
    int64_t ask_px_max = numeric_limits<int64_t>::min();
    long double ask_px_avg = 0.0L;
    uint64_t ask_px_zero = 0;

    bool has_ask_qty = false;
    uint64_t ask_qty_count = 0;
    int64_t ask_qty_min = numeric_limits<int64_t>::max();
    int64_t ask_qty_max = numeric_limits<int64_t>::min();
    long double ask_qty_avg = 0.0L;
    uint64_t ask_qty_zero = 0;

    // helper: counts per-row mode detection
    bool bid_per_row = false;
    bool ask_per_row = false;

    // nulls for scalar columns
    uint64_t null_ts = 0;
    uint64_t null_firstId = 0;
    uint64_t null_lastId = 0;
    uint64_t null_eventTime = 0;
};

static bool analyze_depth_file(const string& path, FileMetric& out)
{
    out = FileMetric();
    out.path = path;
    try {
        unique_ptr<parquet::ParquetFileReader> reader = parquet::ParquetFileReader::OpenFile(path, /*memory_map=*/true);
        auto md = reader->metadata();
        auto schema = md->schema();
        out.meta_rows = md->num_rows();
        out.row_groups = md->num_row_groups();

        // common column names in depth parquet (try multiple possible name variants)
        int idx_ts = find_col_idx(schema, "ts");
        int idx_firstId = find_col_idx(schema, "firstId");
        int idx_lastId  = find_col_idx(schema, "lastId");
        int idx_eventTime = find_col_idx(schema, "eventTime");

        // try common flattened names for bid/ask elements
        int idx_bid_px = find_col_idx(schema, "bid.list.element.px");
        int idx_bid_qty = find_col_idx(schema, "bid.list.element.qty");
        int idx_ask_px = find_col_idx(schema, "ask.list.element.px");
        int idx_ask_qty = find_col_idx(schema, "ask.list.element.qty");

        // also accept shorter names (some exporters flatten differently)
        if (idx_bid_px < 0) idx_bid_px = find_col_idx(schema, "bid_px");
        if (idx_bid_qty < 0) idx_bid_qty = find_col_idx(schema, "bid_qty");
        if (idx_ask_px < 0) idx_ask_px = find_col_idx(schema, "ask_px");
        if (idx_ask_qty < 0) idx_ask_qty = find_col_idx(schema, "ask_qty");

        out.has_ts = (idx_ts >= 0);
        out.has_firstId = (idx_firstId >= 0);
        out.has_lastId = (idx_lastId >= 0);
        out.has_eventTime = (idx_eventTime >= 0);

        out.has_bid_px = (idx_bid_px >= 0);
        out.has_bid_qty = (idx_bid_qty >= 0);
        out.has_ask_px = (idx_ask_px >= 0);
        out.has_ask_qty = (idx_ask_qty >= 0);

        // we'll need per-row firstId/lastId to detect id gaps; read them per row-group
        uint64_t global_rows = 0;
        // For px/qty we may have "flattened" arrays (many values) — we accumulate global stats.
        Welford bid_px_w, bid_qty_w, ask_px_w, ask_qty_w;
        // For id/ts checks we stream row-by-row to keep prev_lastId, prev_ts
        int64_t prev_ts = 0;
        bool have_prev_ts = false;
        uint64_t prev_lastId = 0;
        bool have_prev_lastId = false;

        for (int rg = 0; rg < out.row_groups; ++rg) {
            auto rg_reader = reader->RowGroup(rg);
            int64_t rows = rg_reader->metadata()->num_rows();
            if (rows <= 0) continue;

            vector<int64_t> v_ts, v_first, v_last, v_event;
            if (out.has_ts) read_i64_column(*rg_reader, idx_ts, v_ts);
            if (out.has_firstId) read_i64_column(*rg_reader, idx_firstId, v_first);
            if (out.has_lastId)  read_i64_column(*rg_reader, idx_lastId, v_last);
            if (out.has_eventTime) read_i64_column(*rg_reader, idx_eventTime, v_event);

            // flattened arrays for bid/ask (may be per-row or flattened occurrences)
            vector<int64_t> v_bid_px, v_bid_qty, v_ask_px, v_ask_qty;
            if (out.has_bid_px) read_i64_column(*rg_reader, idx_bid_px, v_bid_px);
            if (out.has_bid_qty) read_i64_column(*rg_reader, idx_bid_qty, v_bid_qty);
            if (out.has_ask_px) read_i64_column(*rg_reader, idx_ask_px, v_ask_px);
            if (out.has_ask_qty) read_i64_column(*rg_reader, idx_ask_qty, v_ask_qty);

            // Determine actual number of rows we can iterate safely in this rowgroup:
            int64_t nrows = rows;
            if (out.has_ts) nrows = min<int64_t>(nrows, (int64_t)v_ts.size());
            if (out.has_firstId) nrows = min<int64_t>(nrows, (int64_t)v_first.size());
            if (out.has_lastId)  nrows = min<int64_t>(nrows, (int64_t)v_last.size());
            if (out.has_eventTime) nrows = min<int64_t>(nrows, (int64_t)v_event.size());

            // If bid_px vector length == nrows => per-row single value
            if (out.has_bid_px && (int64_t)v_bid_px.size() == nrows) out.bid_per_row = true;
            if (out.has_ask_px && (int64_t)v_ask_px.size() == nrows) out.ask_per_row = true;
            if (out.has_bid_qty && (int64_t)v_bid_qty.size() == nrows) out.bid_per_row = true;
            if (out.has_ask_qty && (int64_t)v_ask_qty.size() == nrows) out.ask_per_row = true;

            // Process row-by-row for ts/ids and for per-row bid/ask
            for (int64_t i = 0; i < nrows; ++i) {
                ++out.rows_scanned;
                ++global_rows;

                // ts
                if (out.has_ts) {
                    int64_t t = v_ts[i];
                    if (t < out.ts_min) out.ts_min = t;
                    if (t > out.ts_max) out.ts_max = t;
                    if (have_prev_ts) {
                        uint64_t gap = (t >= prev_ts) ? (uint64_t)(t - prev_ts) : 0;
                        if (gap > out.max_gap_ns) out.max_gap_ns = gap;
                        if (gap >= 100000000ULL) ++out.gaps_gt_100ms;
                        if (gap >= 1000000000ULL) ++out.gaps_gt_1s;
                        if (t < prev_ts) ++out.non_monotonic_ts;
                    }
                    prev_ts = t;
                    have_prev_ts = true;
                } else {
                    ++out.null_ts;
                }

                // firstId/lastId checks
                if (out.has_firstId) {
                    uint64_t fid = (uint64_t)v_first[i];
                    (void)fid;
                } else ++out.null_firstId;
                if (out.has_lastId) {
                    uint64_t lid = (uint64_t)v_last[i];
                    (void)lid;
                } else ++out.null_lastId;

                if (out.has_firstId && out.has_lastId) {
                    uint64_t fid = (uint64_t)v_first[i];
                    uint64_t lid = (uint64_t)v_last[i];
                    if (lid < fid) ++out.last_lt_first_count;

                    if (have_prev_lastId) {
                        if (fid <= prev_lastId) ++out.id_overlap_count;           // overlap
                        if (fid > prev_lastId + 1) ++out.id_gap_count;           // gap (non-contiguous ids)
                    }
                    prev_lastId = (uint64_t)lid;
                    have_prev_lastId = true;
                }

                // per-row bid/ask entries (if present as per-row)
                if (out.bid_per_row) {
                    if (out.has_bid_px) {
                        int64_t bp = v_bid_px[i];
                        bid_px_w.add((long double)bp);
                        if (bp < out.bid_px_min) out.bid_px_min = bp;
                        if (bp > out.bid_px_max) out.bid_px_max = bp;
                        if (bp == 0) ++out.bid_px_zero;
                        ++out.bid_px_count;
                    }
                    if (out.has_bid_qty) {
                        int64_t bq = v_bid_qty[i];
                        bid_qty_w.add((long double)bq);
                        if (bq < out.bid_qty_min) out.bid_qty_min = bq;
                        if (bq > out.bid_qty_max) out.bid_qty_max = bq;
                        if (bq == 0) ++out.bid_qty_zero;
                        ++out.bid_qty_count;
                    }
                } // end per-row bid
                if (out.ask_per_row) {
                    if (out.has_ask_px) {
                        int64_t ap = v_ask_px[i];
                        ask_px_w.add((long double)ap);
                        if (ap < out.ask_px_min) out.ask_px_min = ap;
                        if (ap > out.ask_px_max) out.ask_px_max = ap;
                        if (ap == 0) ++out.ask_px_zero;
                        ++out.ask_px_count;
                    }
                    if (out.has_ask_qty) {
                        int64_t aq = v_ask_qty[i];
                        ask_qty_w.add((long double)aq);
                        if (aq < out.ask_qty_min) out.ask_qty_min = aq;
                        if (aq > out.ask_qty_max) out.ask_qty_max = aq;
                        if (aq == 0) ++out.ask_qty_zero;
                        ++out.ask_qty_count;
                    }
                }
            } // rows in rowgroup

            // If bid/ask arrays are flattened (not per-row), accumulate global stats from values we read
            if (out.has_bid_px && !out.bid_per_row) {
                for (size_t k = 0; k < v_bid_px.size(); ++k) {
                    int64_t bp = v_bid_px[k];
                    bid_px_w.add((long double)bp);
                    if (bp < out.bid_px_min) out.bid_px_min = bp;
                    if (bp > out.bid_px_max) out.bid_px_max = bp;
                    if (bp == 0) ++out.bid_px_zero;
                    ++out.bid_px_count;
                }
            }
            if (out.has_bid_qty && !out.bid_per_row) {
                for (size_t k = 0; k < v_bid_qty.size(); ++k) {
                    int64_t bq = v_bid_qty[k];
                    bid_qty_w.add((long double)bq);
                    if (bq < out.bid_qty_min) out.bid_qty_min = bq;
                    if (bq > out.bid_qty_max) out.bid_qty_max = bq;
                    if (bq == 0) ++out.bid_qty_zero;
                    ++out.bid_qty_count;
                }
            }
            if (out.has_ask_px && !out.ask_per_row) {
                for (size_t k = 0; k < v_ask_px.size(); ++k) {
                    int64_t ap = v_ask_px[k];
                    ask_px_w.add((long double)ap);
                    if (ap < out.ask_px_min) out.ask_px_min = ap;
                    if (ap > out.ask_px_max) out.ask_px_max = ap;
                    if (ap == 0) ++out.ask_px_zero;
                    ++out.ask_px_count;
                }
            }
            if (out.has_ask_qty && !out.ask_per_row) {
                for (size_t k = 0; k < v_ask_qty.size(); ++k) {
                    int64_t aq = v_ask_qty[k];
                    ask_qty_w.add((long double)aq);
                    if (aq < out.ask_qty_min) out.ask_qty_min = aq;
                    if (aq > out.ask_qty_max) out.ask_qty_max = aq;
                    if (aq == 0) ++out.ask_qty_zero;
                    ++out.ask_qty_count;
                }
            }
        } // row groups

        // finalize averages
        out.bid_px_avg = (bid_px_w.n > 0) ? (long double)bid_px_w.mean : 0.0L;
        out.bid_qty_avg = (bid_qty_w.n > 0) ? (long double)bid_qty_w.mean : 0.0L;
        out.ask_px_avg = (ask_px_w.n > 0) ? (long double)ask_px_w.mean : 0.0L;
        out.ask_qty_avg = (ask_qty_w.n > 0) ? (long double)ask_qty_w.mean : 0.0L;

        return true;
    } catch (const exception& e) {
        cerr << "ERROR: open/read failed for " << path << " : " << e.what() << "\n";
        return false;
    }
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " /path/to/parquet_dir output.ndjson [--all]\n";
        return 1;
    }

    cerr << "Depth-file anomaly rules (reported if any match):\n";
    cerr << "  1) rows_scanned == 0 (file empty/unreadable)\n";
    cerr << "  2) rows_scanned != meta_rows (metadata mismatch)\n";
    cerr << "  3) null counts for scalar columns: ts, firstId, lastId, eventTime\n";
    cerr << "  4) non_monotonic_ts > 0 (timestamps go backwards)\n";
    cerr << "  5) lastId < firstId in any row\n";
    cerr << "  6) id overlaps: firstId <= previous.lastId\n";
    cerr << "  7) id gaps: firstId > previous.lastId + 1\n";
    cerr << "  8) bid/ask px == 0 or qty == 0 suspicious counts (large fraction)\n";
    cerr << "  9) inconsistency: bid_px count != bid_qty count (when per-row mapping present)\n";
    cerr << " 10) flattened arrays present without offsets => cannot map per-row elements (informational)\n";
    cerr << " 11) statistical outliers across dataset (max_gap, average px/qty etc.) — not yet implemented here\n\n";

    string dir = argv[1];
    string out_path = argv[2];
    bool write_all = false;
    if (argc >= 4 && string(argv[3]) == "--all") write_all = true;

    vector<string> files;
    for (auto &p : fs::directory_iterator(dir)) {
        if (!p.is_regular_file()) continue;
        if (p.path().extension() == ".parquet") files.push_back(p.path().string());
    }
    if (files.empty()) {
        cerr << "No .parquet files found in " << dir << "\n";
        return 1;
    }

    ofstream fout(out_path);
    if (!fout.is_open()) {
        cerr << "Failed to open output " << out_path << "\n";
        return 1;
    }

    uint64_t idx = 0;
    for (auto &f : files) {
        ++idx;
        cerr << "[" << idx << "/" << files.size() << "] " << f << " ... " << flush;
        FileMetric m;
        bool ok = analyze_depth_file(f, m);
        if (!ok) { cerr << "failed\n"; continue; }
        cerr << "ok (rows=" << m.rows_scanned << ")\n";

        // detect anomalies for depth files
        vector<string> anomalies;

        if (m.rows_scanned == 0) anomalies.push_back("rows_scanned == 0");
        if (m.meta_rows != m.rows_scanned) anomalies.push_back("rows_scanned != meta_rows");
        if (m.null_ts > 0 || m.null_firstId > 0 || m.null_lastId > 0 || m.null_eventTime > 0)
            anomalies.push_back("null_counts > 0 (ts/firstId/lastId/eventTime)");
        if (m.non_monotonic_ts > 0) anomalies.push_back("non_monotonic_ts > 0");
        if (m.last_lt_first_count > 0) anomalies.push_back("lastId < firstId (counted)");
        if (m.id_overlap_count > 0) anomalies.push_back("id_overlap_count > 0 (firstId <= prev.lastId)");
        if (m.id_gap_count > 0) anomalies.push_back("id_gap_count > 0 (firstId > prev.lastId+1)");

        // bid/ask zeros and presence checks
        if (m.has_bid_px && m.bid_px_count == 0) anomalies.push_back("has_bid_px but bid_px_count==0");
        if (m.has_ask_px && m.ask_px_count == 0) anomalies.push_back("has_ask_px but ask_px_count==0");
        if (m.has_bid_qty && m.bid_qty_count == 0) anomalies.push_back("has_bid_qty but bid_qty_count==0");
        if (m.has_ask_qty && m.ask_qty_count == 0) anomalies.push_back("has_ask_qty but ask_qty_count==0");

        // too many zero quantities or zero prices (heuristic: >10% zeros)
        auto pct = [](uint64_t zero, uint64_t total)->double {
            if (total==0) return 0.0;
            return (100.0 * (double)zero) / (double)total;
        };
        if (m.bid_qty_count>0 && pct(m.bid_qty_zero, m.bid_qty_count) > 10.0) anomalies.push_back("high_fraction_bid_qty_zero");
        if (m.ask_qty_count>0 && pct(m.ask_qty_zero, m.ask_qty_count) > 10.0) anomalies.push_back("high_fraction_ask_qty_zero");
        if (m.bid_px_count>0 && pct(m.bid_px_zero, m.bid_px_count) > 10.0) anomalies.push_back("high_fraction_bid_px_zero");
        if (m.ask_px_count>0 && pct(m.ask_px_zero, m.ask_px_count) > 10.0) anomalies.push_back("high_fraction_ask_px_zero");

        // check per-row consistency: if bid_per_row true, ensure counts match rows_scanned
        if (m.bid_per_row && (int64_t)m.bid_px_count != m.rows_scanned && (int64_t)m.bid_qty_count != m.rows_scanned) {
            // if either px or qty per-row count != rows_scanned -> inconsistent
            anomalies.push_back("per-row bid counts mismatch rows_scanned");
        }
        if (m.ask_per_row && (int64_t)m.ask_px_count != m.rows_scanned && (int64_t)m.ask_qty_count != m.rows_scanned) {
            anomalies.push_back("per-row ask counts mismatch rows_scanned");
        }

        // If flattened arrays exist but we couldn't map to per-row, mark informational anomaly
        if ((m.has_bid_px && !m.bid_per_row && m.bid_px_count>0) || (m.has_bid_qty && !m.bid_per_row && m.bid_qty_count>0)
         || (m.has_ask_px && !m.ask_per_row && m.ask_px_count>0) || (m.has_ask_qty && !m.ask_per_row && m.ask_qty_count>0)) {
            anomalies.push_back("flattened_bid_or_ask_arrays_without_offsets (informational)");
        }

        // small-file heuristic
        if (m.meta_rows > 0 && m.meta_rows < 10) anomalies.push_back("meta_rows < 10 (very small depth file)");

        // If no anomalies and not writing all, skip
        if (anomalies.empty() && !write_all) continue;

        // compose JSON
        ostringstream o;
        o << "{";
        o << "\"file\":\"" << m.path << "\"";
        o << ",\"meta_rows\":" << m.meta_rows;
        o << ",\"rows_scanned\":" << m.rows_scanned;
        o << ",\"row_groups\":" << m.row_groups;
        if (m.has_ts) {
            o << ",\"ts_min\":" << m.ts_min << ",\"ts_max\":" << m.ts_max << ",\"max_gap_ns\":" << m.max_gap_ns;
            o << ",\"gaps_gt_100ms\":" << m.gaps_gt_100ms << ",\"gaps_gt_1s\":" << m.gaps_gt_1s << ",\"non_monotonic_ts\":" << m.non_monotonic_ts;
        } else o << ",\"ts_present\":false";
        if (m.has_firstId && m.has_lastId) {
            o << ",\"id_overlap_count\":" << m.id_overlap_count << ",\"id_gap_count\":" << m.id_gap_count << ",\"last_lt_first_count\":" << m.last_lt_first_count;
        } else o << ",\"ids_present\":false";

        // bid/ask summary
        o << ",\"bid_px_count\":" << m.bid_px_count << ",\"bid_px_min\":" << m.bid_px_min << ",\"bid_px_max\":" << m.bid_px_max << ",\"bid_px_avg\":" << fixed << setprecision(6) << (double)m.bid_px_avg << ",\"bid_px_zero\":" << m.bid_px_zero;
        o << ",\"bid_qty_count\":" << m.bid_qty_count << ",\"bid_qty_min\":" << m.bid_qty_min << ",\"bid_qty_max\":" << m.bid_qty_max << ",\"bid_qty_avg\":" << fixed << setprecision(6) << (double)m.bid_qty_avg << ",\"bid_qty_zero\":" << m.bid_qty_zero;
        o << ",\"ask_px_count\":" << m.ask_px_count << ",\"ask_px_min\":" << m.ask_px_min << ",\"ask_px_max\":" << m.ask_px_max << ",\"ask_px_avg\":" << fixed << setprecision(6) << (double)m.ask_px_avg << ",\"ask_px_zero\":" << m.ask_px_zero;
        o << ",\"ask_qty_count\":" << m.ask_qty_count << ",\"ask_qty_min\":" << m.ask_qty_min << ",\"ask_qty_max\":" << m.ask_qty_max << ",\"ask_qty_avg\":" << fixed << setprecision(6) << (double)m.ask_qty_avg << ",\"ask_qty_zero\":" << m.ask_qty_zero;

        o << ",\"null_counts\":{";
        bool firstnc = true;
        if (m.has_ts) { o << "\"ts\":" << m.null_ts; firstnc=false; }
        if (m.has_firstId) { if (!firstnc) o << ","; o << "\"firstId\":" << m.null_firstId; firstnc=false; }
        if (m.has_lastId)  { if (!firstnc) o << ","; o << "\"lastId\":" << m.null_lastId; firstnc=false; }
        if (m.has_eventTime){ if (!firstnc) o << ","; o << "\"eventTime\":" << m.null_eventTime; firstnc=false; }
        o << "}";

        o << ",\"anomalies\":[";
        for (size_t i=0;i<anomalies.size();++i){
            if (i) o << ",";
            o << "\"" << anomalies[i] << "\"";
        }
        o << "]}";
        o << "\n";

        fout << o.str();
    }

    fout.close();
    cerr << "Scan complete. Results: " << out_path << (write_all ? " (all files)" : " (only anomalous files)") << "\n";
    return 0;
}


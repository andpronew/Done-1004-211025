// parquet_bulk_audit_filtered.cpp
// Scan a directory of parquet files and detect anomalies.
// By default writes to output NDJSON only files that *have anomalies*.
// Use optional flag --all to write all files.
//
// Build:
//   g++ -std=gnu++23 -O3 parquet_bulk_audit_filtered.cpp -lparquet -larrow -lzstd -o parquet_bulk_audit
//
// Usage:
//   ./parquet_bulk_audit /path/to/parquet_dir anomalies.ndjson        # only anomalous files
//   ./parquet_bulk_audit /path/to/parquet_dir anomalies_all.ndjson --all  # all files

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

    bool has_ts = false;
    int64_t ts_min = numeric_limits<int64_t>::max();
    int64_t ts_max = numeric_limits<int64_t>::min();
    uint64_t max_gap_ns = 0;
    uint64_t gaps_gt_100ms = 0;
    uint64_t gaps_gt_1s = 0;
    uint64_t non_monotonic_ts = 0;

    bool has_px = false;
    int64_t px_min = numeric_limits<int64_t>::max();
    int64_t px_max = numeric_limits<int64_t>::min();
    long double px_avg = 0.0L;
    uint64_t px_zero_count = 0;

    bool has_qty = false;
    int64_t qty_min = numeric_limits<int64_t>::max();
    int64_t qty_max = numeric_limits<int64_t>::min();
    long double qty_avg = 0.0L;
    uint64_t qty_zero_count = 0;

    bool has_tradeId = false;
    uint64_t dup_tradeid = 0;
    uint64_t tradeid_min = numeric_limits<uint64_t>::max();
    uint64_t tradeid_max = 0;

    uint64_t null_ts = 0;
    uint64_t null_px = 0;
    uint64_t null_qty = 0;
    uint64_t null_tradeId = 0;

    uint64_t ts_samples = 0;
    long double gap_mean = 0.0L;
};

static bool analyze_file(const string& path, FileMetric& out)
{
    out = FileMetric();
    out.path = path;
    try {
        unique_ptr<parquet::ParquetFileReader> reader = parquet::ParquetFileReader::OpenFile(path, /*memory_map=*/true);
        auto md = reader->metadata();
        auto schema = md->schema();
        out.meta_rows = md->num_rows();
        out.row_groups = md->num_row_groups();

        int idx_ts = find_col_idx(schema, "ts");
        int idx_px = find_col_idx(schema, "px");
        int idx_qty = find_col_idx(schema, "qty");
        int idx_tradeId = find_col_idx(schema, "tradeId");

        out.has_ts = (idx_ts >= 0);
        out.has_px = (idx_px >= 0);
        out.has_qty = (idx_qty >= 0);
        out.has_tradeId = (idx_tradeId >= 0);

        Welford gap_w, px_w, qty_w;
        unordered_set<uint64_t> tradeid_seen;
        bool tradeid_overflowed = false;
        const uint64_t TRADEID_UNIQUE_LIMIT = 5'000'000;

        // first pass: basic stats, duplicates, px/qty stats
        for (int rg = 0; rg < out.row_groups; ++rg) {
            auto rg_reader = reader->RowGroup(rg);
            int64_t rows = rg_reader->metadata()->num_rows();
            if (rows <= 0) continue;

            vector<int64_t> v_ts, v_px, v_qty, v_tradeId;
            if (out.has_ts) read_i64_column(*rg_reader, idx_ts, v_ts);
            if (out.has_px) read_i64_column(*rg_reader, idx_px, v_px);
            if (out.has_qty) read_i64_column(*rg_reader, idx_qty, v_qty);
            if (out.has_tradeId) read_i64_column(*rg_reader, idx_tradeId, v_tradeId);

            int64_t nrows = rows;
            if (out.has_ts) nrows = min<int64_t>(nrows, (int64_t)v_ts.size());
            if (out.has_px) nrows = min<int64_t>(nrows, (int64_t)v_px.size());
            if (out.has_qty) nrows = min<int64_t>(nrows, (int64_t)v_qty.size());
            if (out.has_tradeId) nrows = min<int64_t>(nrows, (int64_t)v_tradeId.size());

            for (int64_t i = 0; i < nrows; ++i) {
                ++out.rows_scanned;
                if (out.has_ts) {
                    int64_t t = v_ts[i];
                    ++out.ts_samples;
                    if (t < out.ts_min) out.ts_min = t;
                    if (t > out.ts_max) out.ts_max = t;
                } else ++out.null_ts;

                if (out.has_px) {
                    int64_t p = v_px[i];
                    px_w.add((long double)p);
                    if (p < out.px_min) out.px_min = p;
                    if (p > out.px_max) out.px_max = p;
                    if (p == 0) ++out.px_zero_count;
                } else ++out.null_px;

                if (out.has_qty) {
                    int64_t q = v_qty[i];
                    qty_w.add((long double)q);
                    if (q < out.qty_min) out.qty_min = q;
                    if (q > out.qty_max) out.qty_max = q;
                    if (q == 0) ++out.qty_zero_count;
                } else ++out.null_qty;

                if (out.has_tradeId) {
                    uint64_t tid = static_cast<uint64_t>(v_tradeId[i]);
                    if (!tradeid_overflowed) {
                        if (tradeid_seen.find(tid) != tradeid_seen.end()) ++out.dup_tradeid;
                        else {
                            tradeid_seen.insert(tid);
                            if (tradeid_seen.size() > TRADEID_UNIQUE_LIMIT) { tradeid_overflowed = true; tradeid_seen.clear(); }
                        }
                    }
                    if (tid < out.tradeid_min) out.tradeid_min = tid;
                    if (tid > out.tradeid_max) out.tradeid_max = tid;
                } else ++out.null_tradeId;
            }
        }

        // second pass: compute gaps, gap_mean, non-monotonic
        if (out.has_ts) {
            int64_t prev_ts = 0;
            bool have_prev = false;
            Welford gap_acc;
            unique_ptr<parquet::ParquetFileReader> reader2 = parquet::ParquetFileReader::OpenFile(path, /*memory_map=*/true);
            auto md2 = reader2->metadata();
            int idx_ts2 = find_col_idx(md2->schema(), "ts");
            for (int rg = 0; rg < md2->num_row_groups(); ++rg) {
                auto rg_reader = reader2->RowGroup(rg);
                vector<int64_t> v_ts;
                read_i64_column(*rg_reader, idx_ts2, v_ts);
                for (size_t i = 0; i < v_ts.size(); ++i) {
                    int64_t t = v_ts[i];
                    if (have_prev) {
                        uint64_t gap = (t >= prev_ts) ? (uint64_t)(t - prev_ts) : 0;
                        gap_acc.add((long double)gap);
                        if (gap > out.max_gap_ns) out.max_gap_ns = gap;
                        if (gap >= 100000000ULL) ++out.gaps_gt_100ms;
                        if (gap >= 1000000000ULL) ++out.gaps_gt_1s;
                        if (t < prev_ts) ++out.non_monotonic_ts;
                    }
                    prev_ts = t;
                    have_prev = true;
                }
            }
            out.gap_mean = (gap_acc.n > 0) ? gap_acc.mean : 0.0L;
        }

        out.px_avg = (px_w.n > 0) ? px_w.mean : 0.0L;
        out.qty_avg = (qty_w.n > 0) ? qty_w.mean : 0.0L;

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

    // Print the full list of anomaly rules we consider (for user clarity)
    cerr << "Anomaly rules applied (file will be reported if any match):\n";
    cerr << "  1) rows_scanned == 0 (file unreadable or empty)\n";
    cerr << "  2) rows_scanned != meta_rows (meta mismatch)\n";
    cerr << "  3) dup_tradeid > 0 (duplicate tradeId inside file)\n";
    cerr << "  4) null_counts > 0 for ts/px/qty/tradeId\n";
    cerr << "  5) non_monotonic_ts > 0 (timestamps decreasing)\n";
    cerr << "  6) rows_scanned < 90% of meta_rows (strong shortfall)\n";
    cerr << "  7) statistical outliers (z-score > 3) for rows_ratio, max_gap_ns, gap_mean, px_avg, qty_avg\n";
    cerr << "  8) small file: meta_rows < 100 (possible incomplete download)\n\n";

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

    vector<FileMetric> metrics;
    metrics.reserve(files.size());
    cerr << "Scanning " << files.size() << " files...\n";
    uint64_t idx = 0;
    for (auto &f : files) {
        ++idx;
        cerr << "[" << idx << "/" << files.size() << "] " << f << " ... " << flush;
        FileMetric fm;
        bool ok = analyze_file(f, fm);
        if (ok) {
            metrics.push_back(std::move(fm));
            cerr << "ok (rows=" << metrics.back().rows_scanned << ")\n";
        } else {
            cerr << "failed\n";
        }
    }

    // global statistics
    Welford w_rows_ratio, w_max_gap, w_gap_mean, w_px_avg, w_qty_avg, w_gaps_gt1s;
    for (auto &m : metrics) {
        long double ratio = (m.meta_rows > 0) ? ((long double)m.rows_scanned / (long double)m.meta_rows) : 0.0L;
        w_rows_ratio.add(ratio);
        w_max_gap.add((long double)m.max_gap_ns);
        w_gap_mean.add((long double)m.gap_mean);
        w_px_avg.add((long double)m.px_avg);
        w_qty_avg.add((long double)m.qty_avg);
        w_gaps_gt1s.add((long double)m.gaps_gt_1s);
    }

    const long double Z_THRESH = 3.0L;

    ofstream fout(out_path);
    if (!fout.is_open()) {
        cerr << "Failed to open output " << out_path << "\n";
        return 1;
    }

    // write only anomalous files unless --all
    for (auto &m : metrics) {
        vector<string> anomalies;

        if (m.rows_scanned == 0) anomalies.push_back("rows_scanned == 0");
        if (m.rows_scanned != m.meta_rows) anomalies.push_back("rows_scanned != meta_rows");
        if (m.dup_tradeid > 0) anomalies.push_back("dup_tradeid > 0");
        if (m.null_ts > 0 || m.null_px > 0 || m.null_qty > 0 || m.null_tradeId > 0) anomalies.push_back("null_counts > 0");
        if (m.non_monotonic_ts > 0) anomalies.push_back("non_monotonic_ts > 0");
        if (m.meta_rows > 0 && (long double)m.rows_scanned / (long double)m.meta_rows < 0.9L) anomalies.push_back("rows_scanned < 90% of meta_rows");
        if (m.meta_rows > 0 && m.meta_rows < 100) anomalies.push_back("meta_rows < 100 (small file)");

        auto zscore = [&](const Welford &w, long double val) -> long double {
            long double sd = w.stddev();
            if (sd <= 0.0L) return 0.0L;
            return fabsl((val - w.mean) / sd);
        };

        long double z_rows = (m.meta_rows>0) ? zscore(w_rows_ratio, ((long double)m.rows_scanned/(long double)m.meta_rows)) : 0.0L;
        if (z_rows > Z_THRESH) anomalies.push_back("rows_ratio statistical_outlier");

        long double z_gap = zscore(w_max_gap, (long double)m.max_gap_ns);
        if (z_gap > Z_THRESH) anomalies.push_back("max_gap_ns statistical_outlier");

        long double z_gapmean = zscore(w_gap_mean, (long double)m.gap_mean);
        if (z_gapmean > Z_THRESH) anomalies.push_back("gap_mean statistical_outlier");

        long double z_px = zscore(w_px_avg, (long double)m.px_avg);
        if (m.has_px && z_px > Z_THRESH) anomalies.push_back("px_avg statistical_outlier");

        long double z_qty = zscore(w_qty_avg, (long double)m.qty_avg);
        if (m.has_qty && z_qty > Z_THRESH) anomalies.push_back("qty_avg statistical_outlier");

        // If no anomalies and not asked to write all -> skip
        if (anomalies.empty() && !write_all) continue;

        // Compose JSON
        ostringstream o;
        o << "{";
        o << "\"file\":\"" << m.path << "\"";
        o << ",\"meta_rows\":" << m.meta_rows;
        o << ",\"rows_scanned\":" << m.rows_scanned;
        o << ",\"row_groups\":" << m.row_groups;
        if (m.has_ts) {
            o << ",\"ts_min\":" << m.ts_min << ",\"ts_max\":" << m.ts_max;
            o << ",\"max_gap_ns\":" << m.max_gap_ns;
            o << ",\"gap_mean\":" << fixed << setprecision(3) << (double)m.gap_mean;
            o << ",\"gaps_gt_100ms\":" << m.gaps_gt_100ms << ",\"gaps_gt_1s\":" << m.gaps_gt_1s;
            o << ",\"non_monotonic_ts\":" << m.non_monotonic_ts;
        } else {
            o << ",\"ts_present\":false";
        }
        if (m.has_px) {
            o << ",\"px_min\":" << m.px_min << ",\"px_max\":" << m.px_max << ",\"px_avg\":" << fixed << setprecision(6) << (double)m.px_avg << ",\"px_zero_count\":" << m.px_zero_count;
        } else {
            o << ",\"px_present\":false";
        }
        if (m.has_qty) {
            o << ",\"qty_min\":" << m.qty_min << ",\"qty_max\":" << m.qty_max << ",\"qty_avg\":" << fixed << setprecision(6) << (double)m.qty_avg << ",\"qty_zero_count\":" << m.qty_zero_count;
        } else {
            o << ",\"qty_present\":false";
        }
        if (m.has_tradeId) {
            o << ",\"tradeId_min\":" << (m.tradeid_min==numeric_limits<uint64_t>::max()?0:m.tradeid_min) << ",\"tradeId_max\":" << m.tradeid_max << ",\"dup_tradeid\":" << m.dup_tradeid;
        } else {
            o << ",\"tradeId_present\":false";
        }

        o << ",\"null_counts\":{";
        bool first_nc = true;
        if (m.has_ts) { o << "\"ts\":" << m.null_ts; first_nc = false; }
        if (m.has_px) { if (!first_nc) o << ","; o << "\"px\":" << m.null_px; first_nc = false; }
        if (m.has_qty) { if (!first_nc) o << ","; o << "\"qty\":" << m.null_qty; first_nc = false; }
        if (m.has_tradeId) { if (!first_nc) o << ","; o << "\"tradeId\":" << m.null_tradeId; first_nc = false; }
        o << "}";

        o << ",\"anomalies\":[";
        for (size_t i = 0; i < anomalies.size(); ++i) {
            if (i) o << ",";
            o << "\"" << anomalies[i] << "\"";
        }
        o << "]";

        o << "}\n";
        fout << o.str();
    }

    fout.close();
    cerr << "Scan complete. Results written to " << out_path << (write_all ? " (all files)" : " (only anomalous files)") << "\n";
    return 0;
}


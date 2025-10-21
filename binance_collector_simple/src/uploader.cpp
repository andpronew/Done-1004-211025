#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <filesystem>
#include "external/json.hpp"

using namespace std;
using nlohmann::json;
namespace fs = std::filesystem;

static const string k_config_path = "./config.json";

json load_config()
{
    ifstream in(k_config_path);
    if (!in) { cerr << "ERROR: cannot open config file: " << k_config_path << endl; exit(1); }
    json j; in >> j; return j;
}

string now_timestamp_str()
{
    auto now = chrono::system_clock::now(); time_t t = chrono::system_clock::to_time_t(now);
    char buf[64]; strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", gmtime(&t)); return string(buf);
}

long now_unix_ts() { return (long)chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count(); }

int run_cmd(const string &cmd) { int rc = system(cmd.c_str()); if (rc != 0) cerr << "Command failed (rc="<<rc<<"): "<<cmd<<endl; return rc; }

void upload_file_and_update_manifest(const string &local_path, const string &bucket, const string &prefix, const string &manifest_name, int max_manifest_entries)
{
    fs::path p(local_path);
    string fname = p.filename().string();
    string s3_key = prefix + "/" + fname;
    string s3_uri = "s3://" + bucket + "/" + s3_key;

    string upload_cmd = "aws s3 cp \"" + local_path + "\" \"" + s3_uri + "\" --cache-control \"max-age=31536000\" --acl private";
    if (run_cmd(upload_cmd) != 0) { cerr << "ERROR: upload failed for "<<local_path<<endl; return; }

    string tmp_manifest = "/tmp/manifest.tmp.json";
    string s3_manifest_uri = "s3://" + bucket + "/" + prefix + "/" + manifest_name;
    string download_manifest_cmd = "aws s3 cp \"" + s3_manifest_uri + "\" \"" + tmp_manifest + "\" 2>/dev/null || echo '[]' > \"" + tmp_manifest + "\"";
    run_cmd(download_manifest_cmd);

    json manifest_json;
    try { ifstream fin(tmp_manifest); fin >> manifest_json; fin.close(); } catch(...) { manifest_json = json::array(); }

    json entry; entry["name"] = fname; entry["key"] = s3_key; entry["t"] = now_unix_ts();
    json new_manifest = json::array(); new_manifest.push_back(entry);
    for (const auto &it : manifest_json) { new_manifest.push_back(it); if ((int)new_manifest.size() >= max_manifest_entries) break; }

    ofstream fout(tmp_manifest, ios::trunc); fout << new_manifest.dump(2) << endl; fout.close();
    string upload_manifest_cmd = "aws s3 cp \"" + tmp_manifest + "\" \"" + s3_manifest_uri + "\" --cache-control \"no-cache, no-store, must-revalidate, max-age=0\" --acl private";
    run_cmd(upload_manifest_cmd);

    fs::remove(p);
    fs::remove(tmp_manifest);
    cout << "Uploaded and removed local: " << local_path << " -> " << s3_uri << endl;
}

int main()
{
    json cfg = load_config();
    string bucket = cfg.value("bucket", string());
    string prefix = cfg.value("prefix", string("data"));
    string local_dir = cfg.value("local_dir", string("./local_binance"));
    string manifest_name = cfg.value("manifest_name", string("manifest.json"));
    int sleep_seconds = cfg.value("sleep_seconds", 120);
    int max_manifest_entries = cfg.value("max_manifest_entries", 500);

    if (bucket.empty()) { cerr << "ERROR: bucket not set in config.json" << endl; return 1; }
    fs::create_directories(local_dir);

    cout << "Uploader started. Local dir: " << local_dir << ", bucket: " << bucket << endl;

    while (true)
    {
        string ts = now_timestamp_str();
        string fname = "bn_" + ts + ".json";
        string raw_path = local_dir + "/" + fname;
        ofstream fout(raw_path, ios::trunc);
        fout << "{\"ts\":\"" << ts << "\", \"sample\": true}" << endl;
        fout.close();

        string gzip_cmd = "gzip -f \"" + raw_path + "\"";
        if (run_cmd(gzip_cmd) != 0) { cerr << "ERROR: gzip failed for " << raw_path << endl; this_thread::sleep_for(chrono::seconds(sleep_seconds)); continue; }
        string gz_path = raw_path + ".gz";

        upload_file_and_update_manifest(gz_path, bucket, prefix, manifest_name, max_manifest_entries);
        this_thread::sleep_for(chrono::seconds(sleep_seconds));
    }
}

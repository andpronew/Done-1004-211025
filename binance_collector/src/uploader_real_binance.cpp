// uploader_real_binance.cpp
// Fetch trades from Binance (signed if api_key/api_secret provided) and upload .gz files to S3.
// Uses libcurl and OpenSSL HMAC-SHA256 for signatures.
// Strings/messages in English (per your policy). Config + paths in config.json.

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <iomanip>

#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <curl/curl.h>

#include <nlohmann/json.hpp>

using namespace std;
using nlohmann::json;
namespace fs = std::filesystem;

static const string k_config_path = "./config.json";

// helper: load config
json load_config()
{
    ifstream in(k_config_path);
    if (!in)
    {
        cerr << "ERROR: cannot open config file: " << k_config_path << endl;
        exit(1);
    }
    json j; in >> j; return j;
}

// helper: current ts for filenames
string now_timestamp_str()
{
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", gmtime(&t));
    return string(buf);
}

// return current unix time in milliseconds (helper)
long long now_ms()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// helper: unix

// HMAC-SHA256 returning hex string
string hmac_sha256_hex(const string &key, const string &data)
{
    unsigned char *result;
    unsigned int len = EVP_MAX_MD_SIZE;
    result = (unsigned char*)malloc(len);
    HMAC_CTX *ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, key.c_str(), (int)key.size(), EVP_sha256(), NULL);
    HMAC_Update(ctx, (unsigned char*)data.c_str(), data.size());
    HMAC_Final(ctx, result, &len);
    HMAC_CTX_free(ctx);

    // hex
    stringstream ss;
    for (unsigned int i = 0; i < len; ++i)
        ss << hex << setw(2) << setfill('0') << (int)result[i];
    free(result);
    return ss.str();
}

// curl write func to string
static size_t write_to_string(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    auto real_size = size * nmemb;
    string *s = (string*)userdata;
    s->append((char*)ptr, real_size);
    return real_size;
}

// perform HTTP GET with optional headers, returns true + response in out_resp on success
bool http_get(const string &url, const vector<string> &headers, string &out_resp, long timeout_sec = 30)
{
    CURL *curl = curl_easy_init();
    if (!curl) return false;
    struct curl_slist *hlist = NULL;
    for (auto &h : headers) hlist = curl_slist_append(hlist, h.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hlist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out_resp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_sec);
    // user agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "BinanceCollector/1.0");
    CURLcode rc = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(hlist);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) return false;
    if (http_code >= 400) return false;
    return true;
}

// run system command (returns rc)
int run_cmd(const string &cmd)
{
    int rc = system(cmd.c_str());
    if (rc != 0)
        cerr << "Command failed (rc=" << rc << "): " << cmd << endl;
    return rc;
}

// upload via aws cli and update manifest (same approach as earlier)
void upload_file_and_update_manifest(const string &local_path,
                                     const string &bucket,
                                     const string &prefix,
                                     const string &manifest_name,
                                     int max_manifest_entries)
{
    fs::path p(local_path);
    string fname = p.filename().string();
    string s3_key = prefix + "/" + fname;
    string s3_uri = "s3://" + bucket + "/" + s3_key;

    string upload_cmd = "aws s3 cp \"" + local_path + "\" \"" + s3_uri + "\" --cache-control \"max-age=31536000\" --acl private";
    if (run_cmd(upload_cmd) != 0) { cerr << "ERROR: upload failed for " << local_path << endl; return; }

    string tmp_manifest = "/tmp/manifest.tmp.json";
    string s3_manifest_uri = "s3://" + bucket + "/" + prefix + "/" + manifest_name;
    string download_manifest_cmd = "aws s3 cp \"" + s3_manifest_uri + "\" \"" + tmp_manifest + "\" 2>/dev/null || echo '[]' > \"" + tmp_manifest + "\"";
    run_cmd(download_manifest_cmd);

    json manifest_json;
    try { ifstream fin(tmp_manifest); fin >> manifest_json; fin.close(); } catch(...) { manifest_json = json::array(); }

    json entry;
    entry["name"] = fname;
    entry["key"] = s3_key;
    entry["t"] = (long long)(now_ms());

    json new_manifest = json::array();
    new_manifest.push_back(entry);
    for (const auto &it : manifest_json) {
        new_manifest.push_back(it);
        if ((int)new_manifest.size() >= max_manifest_entries) break;
    }

    ofstream fout(tmp_manifest, ios::trunc);
    fout << new_manifest.dump(2) << endl;
    fout.close();

    string upload_manifest_cmd = "aws s3 cp \"" + tmp_manifest + "\" \"" + s3_manifest_uri + "\" --cache-control \"no-cache, no-store, must-revalidate, max-age=0\" --acl private";
    run_cmd(upload_manifest_cmd);

    // remove local source to free disk
    fs::remove(p);
    fs::remove(tmp_manifest);

    cout << "Uploaded and removed local: " << local_path << " -> " << s3_uri << endl;
}

int main()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);

    json cfg = load_config();
    string bucket = cfg.value("bucket", string());
    string prefix = cfg.value("prefix", string("data"));
    string local_dir = cfg.value("local_dir", string("./local_binance"));
    string manifest_name = cfg.value("manifest_name", string("manifest.json"));
    int sleep_seconds = cfg.value("sleep_seconds", 120);
    int max_manifest_entries = cfg.value("max_manifest_entries", 500);
    string symbol = cfg.value("symbol", string("BTCUSDT"));
    bool use_signed = cfg.value("use_signed_api", false);
    string api_key = cfg.value("api_key", string());
    string api_secret = cfg.value("api_secret", string());
    string binance_base = cfg.value("binance_base", string("https://api.binance.com"));

        if (bucket.empty()) { cerr << "ERROR: bucket not set in config.json" << endl; return 1; }
    fs::create_directories(local_dir);

    cout << "Uploader started. Local dir: " << local_dir << ", bucket: " << bucket << ", symbol: " << symbol << endl;

    while (true)
    {
        // Build endpoint and query
// --- build endpoint and fetch data (supports signed private endpoint myTrades using serverTime) ---
// --- build endpoint and fetch data (supports signed private endpoint myTrades using serverTime) ---
string resp;
bool ok = false;
long long server_ts_ms = 0;
if (use_signed)
{
    // 1. получить serverTime с Binance
    string time_resp;
    if (http_get(binance_base + "/api/v3/time", {}, time_resp))
    {
        auto j = nlohmann::json::parse(time_resp);
        server_ts_ms = j["serverTime"].get<long long>();
    }
    else
    {
        cerr << "ERROR: cannot fetch server time" << endl;
        return 1;
    }

    // 2. собрать query string
    string qs = "symbol=" + symbol +
                "&timestamp=" + to_string(server_ts_ms) +
                "&recvWindow=5000";

    // 3. подписать запрос
    string sig = hmac_sha256_hex(api_secret, qs);

    // 4. полный URL
    string url = binance_base + "/api/v3/myTrades?" + qs + "&signature=" + sig;

    // 5. заголовки
    vector<string> headers = {"X-MBX-APIKEY: " + api_key};

    cout << "DEBUG: requesting private myTrades: " << url << endl;

    string resp;
    if (!http_get(url, headers, resp))
    {
        cerr << "ERROR: myTrades request failed" << endl;
        return 1;
    }

    // здесь `resp` содержит JSON с приватными сделками
}
else
{
    string url = binance_base + "/api/v3/aggTrades?symbol=" + symbol + "&limit=1000";

    cout << "DEBUG: requesting public aggTrades: " << url << endl;

    string resp;
    if (!http_get(url, {}, resp))
    {
        cerr << "ERROR: aggTrades request failed" << endl;
        return 1;
    }

    // здесь `resp` содержит JSON с публичными сделками
}




// now `ok` == true means resp contains Binance JSON

// write response to file
        string ts = now_timestamp_str();
        string fname = "bn_" + symbol + "_" + ts + ".json";
        string path = local_dir + "/" + fname;
        ofstream out(path, ios::binary);
        out << resp;
        out.close();

        // gzip the file (uses system gzip)
        string gzip_cmd = "gzip -f \"" + path + "\"";
        if (run_cmd(gzip_cmd) != 0)
        {
            cerr << "ERROR: gzip failed for " << path << endl;
            // cleanup and continue
            fs::remove(path);
            this_thread::sleep_for(chrono::seconds(sleep_seconds));
            continue;
        }
        string gz_path = path + ".gz";

        // upload & update manifest
        upload_file_and_update_manifest(gz_path, bucket, prefix, manifest_name, max_manifest_entries);

        // sleep
        this_thread::sleep_for(chrono::seconds(sleep_seconds));
    }

    curl_global_cleanup();
    return 0;
}


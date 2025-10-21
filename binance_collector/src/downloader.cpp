#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <filesystem>
#include <nlohmann/json.hpp>


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


int run_cmd(const string &cmd) { int rc = system(cmd.c_str()); if (rc != 0) cerr << "Command failed (rc="<<rc<<"): "<<cmd<<endl; return rc; }


int main()
{
json cfg = load_config();
string cf_domain = cfg.value("cloudfront_domain", string());
string prefix = cfg.value("prefix", string("data"));
string download_dir = cfg.value("download_dir", string("./downloaded"));
string state_file = cfg.value("state_file", string("./.bn_downloaded"));
int sleep_seconds = cfg.value("sleep_seconds", 120);


if (cf_domain.empty()) { cerr << "ERROR: cloudfront_domain not set in config.json" << endl; return 1; }
fs::create_directories(download_dir);
{ ofstream of(state_file, ios::app); }


cout << "Downloader started. CloudFront: " << cf_domain << ", download dir: " << download_dir << endl;


while (true)
{
string manifest_url = "https://" + cf_domain + "/" + prefix + "/manifest.json";
string tmp_manifest = "/tmp/manifest.cf.json";
string curl_cmd = "curl -sSf \"" + manifest_url + "\" -o \"" + tmp_manifest + "\"";
if (run_cmd(curl_cmd) != 0) { cerr << "ERROR: failed to fetch manifest from "<<manifest_url<<endl; this_thread::sleep_for(chrono::seconds(30)); continue; }


json manifest;
try { ifstream fin(tmp_manifest); fin >> manifest; fin.close(); } catch(...) { cerr << "ERROR: parsing manifest failed"<<endl; fs::remove(tmp_manifest); this_thread::sleep_for(chrono::seconds(sleep_seconds)); continue; }


unordered_set<string> downloaded;
{ ifstream sf(state_file); string line; while (getline(sf, line)) if (!line.empty()) downloaded.insert(line); }


if (!manifest.is_array()) { cerr << "ERROR: manifest not an array"<<endl; fs::remove(tmp_manifest); this_thread::sleep_for(chrono::seconds(sleep_seconds)); continue; }


for (auto it = manifest.rbegin(); it != manifest.rend(); ++it)
{
if (!it->contains("key")) continue;
string key = (*it)["key"].get<string>();
string fname = fs::path(key).filename().string();
if (downloaded.find(fname) != downloaded.end()) continue;


string url = "https://" + cf_domain + "/" + key;
string local_path = download_dir + "/" + fname;
if (fs::exists(local_path)) { ofstream ofs(state_file, ios::app); ofs << fname << "\n"; ofs.close(); downloaded.insert(fname); cout << "Already present locally: "<<local_path<<endl; continue; }


cout << "Downloading "<<url<<" -> "<<local_path<<endl;
string curl_dl_cmd = "curl -f -L -C - -o \"" + local_path + "\" \"" + url + "\"";
if (run_cmd(curl_dl_cmd) == 0) { ofstream ofs(state_file, ios::app); ofs << fname << "\n"; ofs.close(); downloaded.insert(fname); cout << "Downloaded: "<<fname<<endl; }
else { cerr << "ERROR: failed to download "<<url<<endl; }
}


fs::remove(tmp_manifest);
this_thread::sleep_for(chrono::seconds(sleep_seconds));
}
return 0;
}

#include <iostream>
#include <curl/curl.h>
#include <string>
#include <nlohmann/json.hpp>
#include <ctime>

using namespace std;
using json = nlohmann::json;

static size_t write_callback(void* contents, size_t size, size_t nmemb, string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

string get_address_info(const string& address) {
    CURL* curl = curl_easy_init();
    string response;

    if (curl) {
        string url = "https://blockstream.info/api/address/" + address + "/txs";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
        curl_easy_cleanup(curl);
    }

    return response;
}

int main() {
    cout << "Bitcoin-address: ";
    string address;
    cin >> address;

    string json_data = get_address_info(address);

    if (json_data.empty() || json_data.find("error") != string::npos) {
        cerr << "Error fetching or invalid data: " << json_data << endl;
        return 1;
    }

    json parsed = json::parse(json_data);

    for (const auto& tx : parsed) {
        cout << "TXID: " << tx["txid"] << "\n";

        // Размер — vsize, но может отсутствовать, проверим
        int vsize = 0;
        if (tx.contains("vsize") && tx["vsize"].is_number()) {
            vsize = tx["vsize"];
        }
        cout << "Размер: " << vsize << " vB\n";

        if (tx.contains("status")) {
            const auto& status = tx["status"];

            if (status.contains("block_height")) {
                cout << "Блок: " << status["block_height"] << "\n";
            } else {
                cout << "Блок: (нет данных)\n";
            }

            if (status.contains("block_time")) {
                time_t timestamp = status["block_time"];
                tm* timeinfo = gmtime(&timestamp);
                char buffer[80];
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S UTC", timeinfo);
                cout << "Время: " << buffer << "\n";
            } else {
                cout << "Время: (нет данных)\n";
            }
        } else {
            cout << "Статус транзакции отсутствует\n";
        }

        cout << "--------------------\n";
    }

    return 0;
}


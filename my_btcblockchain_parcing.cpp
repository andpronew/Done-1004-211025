#include <iostream>
#include <curl/curl.h>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

std::string get_address_info(const std::string& address) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        std::string url = "https://blockstream.info/api/address/" + address + "/txs";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    return response;
}

int main() {
    std::cout << "Bitcoin-address: ";
    std::string address;
    std::cin >> address;
    std::string json_data = get_address_info(address);

    // std::cout << "Raw JSON:\n" << json_data << std::endl;
    
    if (json_data.empty() || json_data.find("error") != std::string::npos) {
    std::cerr << "Error fetching or invalid data: " << json_data << std::endl;
    return 1;
}

    json parsed = json::parse(json_data);

    int weight = tx.value("weight", 0);
    int vsize = (weight + 3) / 4;

    std::cout << "Вес: " << weight << " (vsize ≈ " << vsize << " vB)\n";
   for (const auto& tx : parsed) {
    std::cout << "TXID: " << tx.value("txid", "(нет txid)") << "\n";
    std::cout << "Размер: " << tx.value("vsize", 0) << " vB\n";

    if (tx.contains("status") && tx["status"].is_object()) {
        const auto& status = tx["status"];
        std::cout << "Блок: " << status.value("block_height", -1) << "\n";
        std::cout << "Время: " << status.value("block_time", -1) << "\n";
    } else {
        std::cout << "Статус транзакции отсутствует\n";
    }

    std::cout << "--------------------\n";
}

    return 0;
}


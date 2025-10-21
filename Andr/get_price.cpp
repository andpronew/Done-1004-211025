#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <iostream>
#include <locale>

using namespace web;
using namespace web::http;
using namespace web::http::client;

int main() {
    try {
        http_client client(U("https://api.binance.com"));
        uri_builder builder(U("/api/v3/ticker/price"));
        builder.append_query(U("symbol"), U("BTCUSDT"));

        client.request(methods::GET, builder.to_string())
            .then([](http_response response) {
                if (response.status_code() == status_codes::OK) {
                    return response.extract_json();
                }
                throw std::runtime_error("Не удалось получить ответ");
            })
            .then([](json::value json) {
                std::wcout.imbue(std::locale(""));
                std::wcout << L"Цена: " << json.at(U("price")).as_string() << std::endl;
            })
            .wait();
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
    }
    return 0;
}

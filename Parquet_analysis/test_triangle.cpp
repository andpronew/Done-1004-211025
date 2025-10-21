#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

using namespace std;

// Вызов основной логики из оригинального файла (предполагаем, что она обёрнута в функцию)
int run_triangle_strategy(const string& BTCUSDT_6_test, const string& BTCUSDT_6_test, const string& , const string& output_file);

TEST_CASE("Integration test on known small input") {
    string f1 = "test_btc_usdt.csv";
    string f2 = "test_xtz_btc.csv";
    string f3 = "test_xtz_usdt.csv";
    string out = "test_result.csv";

    // Убедимся, что файл существует
    CHECK(run_triangle_strategy(f1, f2, f3, out) == 0);

    // Читаем результат
    ifstream result(out);
    REQUIRE(result.is_open());

    string line;
    getline(result, line);  // Первая строка

    // Примерная проверка: должно быть 2 строки (2 временных отсчета)
    int lines = 0;
    while (getline(result, line)) {
        lines++;
    }

    CHECK(lines == 2);

    // Пример точной проверки значений (если вы знаете точные формулы)
    result.clear();
    result.seekg(0);
    getline(result, line); // заголовок
    getline(result, line); // первая строка
    CHECK(line.find("551.9") != string::npos); // XTZUSDT_bid = 0.5519
}




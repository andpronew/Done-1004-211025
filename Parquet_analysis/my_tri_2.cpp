#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <ctime>

using namespace std;

struct Tick {
    long long timestamp_ns; // время в наносекундах
    double ask_price;       // цена ask в сатошах
    double ask_volume;      // объём ask в сатошах
    double bid_price;       // цена bid в сатошах
    double bid_volume;      // объём bid в сатошах
};

// Преобразование наносекунд в строку времени
string format_time(long long ns) {
    time_t sec = ns / 1'000'000'000LL;
    tm *ptm = gmtime(&sec);
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ptm);
    return string(buffer);
}

// Чтение тиковых данных из кастомного CSV
vector<Tick> read_tick_csv(const string &filename) {
    ifstream file(filename);
    vector<Tick> data;
    string line;

    while (getline(file, line)) {
        stringstream ss(line);
        string token;
        vector<string> fields;

        while (getline(ss, token, ',')) {
            fields.push_back(token);
        }

        if (fields.size() < 6) continue;

        try {
            Tick t;
            t.timestamp_ns = stoll(fields[0]);
            t.ask_price    = stod(fields[1]) / 1e8;
            t.ask_volume   = stod(fields[2]) / 1e8;
            t.bid_price    = stod(fields[3]) / 1e8;
            t.bid_volume   = stod(fields[4]) / 1e8;
            data.push_back(t);
        } catch (...) {
            continue;
        }
    }

    return data;
}

int main() {
    double capital = 1000.0;
    double fee = 0.00075; // 0.075% комиссия на каждую операцию

    vector<Tick> btcusdt = read_tick_csv("BTCUSDT_4.csv");
    vector<Tick> xtzbtc  = read_tick_csv("XTZBTC_4.csv");
    vector<Tick> xtzusdt = read_tick_csv("XTZUSDT_4.csv");
    size_t n = min({btcusdt.size(), xtzbtc.size(), xtzusdt.size()});

    ofstream fout("result.csv");
    fout << "Time,BTCUSDT,XTZBTC,XTZUSDT,Final_USDT_Direct,Profit_Direct,Final_USDT_Reverse,Profit_Reverse\n";
    fout << fixed << setprecision(7);

    double total_profit_direct = 0.0;
    double total_profit_reverse = 0.0;

    for (size_t i = 0; i < n; ++i) {
        const auto &b = btcusdt[i];
        const auto &x = xtzbtc[i];
        const auto &z = xtzusdt[i];

        // Прямой путь: USDT → BTC (по ask) → XTZ (по ask) → USDT (по bid)
        double usdt_to_btc = capital / b.ask_price * (1 - fee);
        double btc_to_xtz  = usdt_to_btc / x.ask_price * (1 - fee);
        double xtz_to_usdt = btc_to_xtz * z.bid_price * (1 - fee);
        double profit_direct = xtz_to_usdt - capital;

        // Обратный путь: USDT → XTZ (по ask) → BTC (по bid) → USDT (по bid)
        double usdt_to_xtz2 = capital / z.ask_price * (1 - fee);
        double xtz_to_btc2  = usdt_to_xtz2 * x.bid_price * (1 - fee);
        double btc_to_usdt2 = xtz_to_btc2 * b.bid_price * (1 - fee);
        double profit_reverse = btc_to_usdt2 - capital;

        if (profit_direct > 0 || profit_reverse > 0) {
            fout << format_time(b.timestamp_ns) << ","
                 << b.bid_price << "," << x.bid_price << "," << z.bid_price << ","
                 << "," << profit_direct << ","
                 << profit_reverse << "\n";

            total_profit_direct += profit_direct;
            total_profit_reverse += profit_reverse;
        }
    }

    fout.close();

    cout << "Saved results to result.csv" << endl;
    cout << "Total profit (direct):  " << total_profit_direct << " USDT" << endl;
    cout << "Total profit (reverse): " << total_profit_reverse << " USDT" << endl;

    return 0;
}


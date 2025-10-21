#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <ctime>

using namespace std;

struct Candle {
    long long timestamp;
    double open, high, low, close;
};

vector<Candle> read_csv(const string &filename) {
    ifstream file(filename);
    vector<Candle> data;
    string line;

    while (getline(file, line)) {
        stringstream ss(line);
        string token;
        Candle c;
        int column = 0;

        while (getline(ss, token, ',') && column < 5) {
            switch (column) {
                case 0: c.timestamp = stoll(token); break;
                case 1: c.open = stod(token); break;
                case 2: c.high = stod(token); break;
                case 3: c.low  = stod(token); break;
                case 4: c.close = stod(token); break;
            }
            ++column;
        }

        if (column >= 5) {
            data.push_back(c);
        }
    }

    return data;
}

string format_time(long long ms) {
    time_t sec = ms / 1000000;
    tm *ptm = gmtime(&sec);
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ptm);
    return string(buffer);
}

int main() {
    double capital = 1000.0;
    double fee = 0.00075; // 0.075%

    vector<Candle> btcusdt = read_csv("BTCUSDT_4.csv");
    vector<Candle> xtzbtc  = read_csv("XTZBTC_4.csv");
    vector<Candle> xtzusdt = read_csv("XTZUSDT_4.csv");

    size_t n = min({btcusdt.size(), xtzbtc.size(), xtzusdt.size()});

    ofstream fout("result.csv");
    fout << "Time,BTCUSDT,XTZBTC,XTZUSDT,Final_USDT_Direct,Profit_Direct,Final_USDT_Reverse,Profit_Reverse\n";
    fout << fixed << setprecision(5);

    double total_profit_direct = 0.0;
    double total_profit_reverse = 0.0;

    for (size_t i = 0; i < n; ++i) {
        const auto &b = btcusdt[i];
        const auto &x = xtzbtc[i];
        const auto &z = xtzusdt[i];

        double rate1 = capital / b.close * (1 - fee);        // USDT -> BTC
        double rate2 = rate1 / x.close * (1 - fee);          // BTC -> XTZ
        double rate3 = rate2 * z.close * (1 - fee);          // XTZ -> USDT
        double profit_direct = rate3 - capital;

        double rev1 = capital / z.close * (1 - fee);         // USDT -> XTZ
        double rev2 = rev1 * x.close * (1 - fee);            // XTZ -> BTC
        double rev3 = rev2 * b.close * (1 - fee);            // BTC -> USDT
        double profit_reverse = rev3 - capital;

        // Фильтрация по прибыли
        if (profit_direct > 0 || profit_reverse > 0) {
            fout << format_time(b.timestamp) << "," << b.close << "," << x.close << "," << z.close << ","
                 << rate3 << "," << profit_direct << ","
                 << rev3 << "," << profit_reverse << "\n";

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


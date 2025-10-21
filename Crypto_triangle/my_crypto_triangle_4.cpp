#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <chrono>

using namespace std;

struct Record {
    string human_time;
    double result;
    double btcusdt_price;
    double bnbbtc_price;
    double bnbusdt_price;
};

vector<string> file_list_btcusdt = {
    "BTCUSDT-1s-2025-07-11.csv",
    "BTCUSDT-1s-2025-07-12.csv",
    "BTCUSDT-1s-2025-07-13.csv",
    "BTCUSDT-1s-2025-07-14.csv"
};

vector<string> file_list_bnbbtc = {
    "XTZBTC-1s-2025-07-11.csv",
    "XTZBTC-1s-2025-07-12.csv",
    "XTZBTC-1s-2025-07-13.csv",
    "XTZBTC-1s-2025-07-14.csv"
};

vector<string> file_list_bnbusdt = {
    "XTZUSDT-1s-2025-07-11.csv",
    "XTZUSDT-1s-2025-07-12.csv",
    "XTZUSDT-1s-2025-07-13.csv",
    "XTZUSDT-1s-2025-07-14.csv"
};

vector<double> load_column(const vector<string>& files, int column_index) {
    vector<double> values;
    for (const auto& file : files) {
        ifstream in(file);
        if (!in.is_open()) {
            cerr << "Failed to open file: " << file << endl;
            continue;
        }
        string line;
        while (getline(in, line)) {
            stringstream ss(line);
            string cell;
            for (int i = 0; i <= column_index; ++i) {
                getline(ss, cell, ',');
            }
            values.push_back(stod(cell));
        }
    }
    return values;
}

vector<long long> load_timestamps(const vector<string>& files) {
    vector<long long> timestamps;
    for (const auto& file : files) {
        ifstream in(file);
        if (!in.is_open()) {
            cerr << "Failed to open file: " << file << endl;
            continue;
        }
        string line;
        while (getline(in, line)) {
            stringstream ss(line);
            string ts_str;
            getline(ss, ts_str, ',');
            timestamps.push_back(stoll(ts_str));
        }
    }
    return timestamps;
}

string to_human_time(long long millis) {
    time_t seconds = millis / 1000000;
    tm* tm_ptr = gmtime(&seconds);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_ptr);
    return string(buffer);
}

int main() {
    double capital;
    double fee_btcusdt, fee_bnbbtc, fee_bnbusdt;

    cout << "Enter initial capital in USDT: ";
    cin >> capital;
    cout << "Enter fee for BTCUSDT (e.g., 0.001 for 0.1%): ";
    cin >> fee_btcusdt;
    cout << "Enter fee for XTZBTC: ";
    cin >> fee_bnbbtc;
    cout << "Enter fee for XTZUSDT: ";
    cin >> fee_bnbusdt;

    vector<long long> timestamps = load_timestamps(file_list_btcusdt);
    vector<double> btcusdt = load_column(file_list_btcusdt, 4);
    vector<double> bnbbtc = load_column(file_list_bnbbtc, 4);
    vector<double> bnbusdt = load_column(file_list_bnbusdt, 4);

    size_t min_size = min({btcusdt.size(), bnbbtc.size(), bnbusdt.size(), timestamps.size()});
    vector<Record> profitable;

    cout << "Using fee_btcusdt = " << fee_btcusdt << endl;

    for (size_t i = 0; i < min_size; ++i) {
        double step1 = capital / btcusdt[i] * (1.0 - fee_btcusdt);
        double step2 = step1 / bnbbtc[i] * (1.0 - fee_bnbbtc);
        double result = step2 * bnbusdt[i] * (1.0 - fee_bnbusdt) - capital;

        if (result > 0.0) { // Только положительные сделки
            Record rec;
            rec.human_time = to_human_time(timestamps[i]);
            rec.result = result;
            rec.btcusdt_price = btcusdt[i];
            rec.bnbbtc_price = bnbbtc[i];
            rec.bnbusdt_price = bnbusdt[i];
            profitable.push_back(rec);
        }

    }
    sort(profitable.begin(), profitable.end(), [](const Record& a, const Record& b) {
        return a.result > b.result;
    });

    double total = 0.0;
    for (const auto& rec : profitable) {
        total += rec.result;
    }

    cout << fixed << setprecision(8);
    cout << "TOTAL PROFITABLE SUM: " << total << "\n";

    ofstream out("result.csv");
    out << "Time,Result,BTCUSDT,BNBBTC,BNBUSDT\n";
    for (const auto& r : profitable) {
    out << r.human_time << "," << r.result << ","
        << r.btcusdt_price << "," << r.bnbbtc_price << "," << r.bnbusdt_price << "\n";
}
out.close();

    return 0;
}


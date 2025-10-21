#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <map>
#include <algorithm>
#include <ctime>

using namespace std;

struct Record {
    string human_time;
    double result;
};

// Преобразование timestamp в человекочитаемый формат
string to_human_time(long long timestamp_ms) {
    time_t seconds = timestamp_ms / 1000000;
    tm* timeinfo = gmtime(&seconds);
    char buffer[25];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return string(buffer);
}

// Загрузка всех файлов одной пары
vector<pair<long long, double>> load_closes(const vector<string>& filenames) {
    vector<pair<long long, double>> data;
    for (const auto& filename : filenames) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Failed to open: " << filename << endl;
            continue;
        }
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string field;
            vector<string> tokens;
            while (getline(ss, field, ',')) {
                tokens.push_back(field);
            }
            if (tokens.size() < 7) continue;
            long long ts = stoll(tokens[0]);
            double close = stod(tokens[4]);
            data.emplace_back(ts, close);
        }
    }
    sort(data.begin(), data.end()); // сортировка по времени
    return data;
}

int main() {
    // Имя файлов для каждой пары
    vector<string> btcusdt_files = {
        "BTCUSDT-1s-2025-07-11.csv", "BTCUSDT-1s-2025-07-12.csv",
        "BTCUSDT-1s-2025-07-13.csv", "BTCUSDT-1s-2025-07-14.csv"
    };
    vector<string> bnbbtc_files = {
        "XTZBTC-1s-2025-07-11.csv", "XTZBTC-1s-2025-07-12.csv",
        "XTZBTC-1s-2025-07-13.csv", "XTZBTC-1s-2025-07-14.csv"
    };
    vector<string> bnbusdt_files = {
        "XTZUSDT-1s-2025-07-11.csv", "XTZUSDT-1s-2025-07-12.csv",
        "XTZUSDT-1s-2025-07-13.csv", "XTZUSDT-1s-2025-07-14.csv"
    };

    double capital = 1000.0;
    double fee_btcusdt, fee_bnbbtc, fee_bnbusdt;

    cout << "Enter fee for BTCUSDT (e.g. 0.001 for 0.1%): ";
    cin >> fee_btcusdt;
    cout << "Enter fee for BNBBTC: ";
    cin >> fee_bnbbtc;
    cout << "Enter fee for BNBUSDT: ";
    cin >> fee_bnbusdt;

    auto btcusdt = load_closes(btcusdt_files);
    auto bnbbtc = load_closes(bnbbtc_files);
    auto bnbusdt = load_closes(bnbusdt_files);

    size_t total = min({btcusdt.size(), bnbbtc.size(), bnbusdt.size()});
    vector<Record> records;
    double sum_results = 0.0;

    for (size_t i = 0; i < total; ++i) {
        long long ts1 = btcusdt[i].first;
        long long ts2 = bnbbtc[i].first;
        long long ts3 = bnbusdt[i].first;

        if (ts1 != ts2 || ts1 != ts3) continue;

        double btcusdt_close = btcusdt[i].second;
        double bnbbtc_close = bnbbtc[i].second;
        double bnbusdt_close = bnbusdt[i].second;

        double result = (((capital
                          / btcusdt_close) * (1.0 - fee_btcusdt))
                          / bnbbtc_close) * (1.0 - fee_bnbbtc)
                          * (bnbusdt_close * (1.0 - fee_bnbusdt)) - capital;

        string human_time = to_human_time(ts1);
        records.push_back({human_time, result});
        sum_results += result;
    }

    // Сортировка по убыванию result
    sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
        return a.result > b.result;
    });

    ofstream out("result.csv");
    out << fixed << setprecision(6);
    out << "Total Result:," << sum_results << "\n";
    out << "Time,Result\n";

    for (const auto& r : records) {
        out << r.human_time << "," << r.result << "\n";
    }

    cout << "Done. Output written to result.csv\n";
    return 0;
}


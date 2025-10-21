#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <cmath>

using namespace std;

using Timestamp = long long;

struct Candle {
    Timestamp timestamp;
    double close;
};

map<Timestamp, double> load_data(const vector<string>& filenames) {
    map<Timestamp, double> data;
    for (const string& filename : filenames) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Failed to open file: " << filename << endl;
            continue;
        }
        string line;
        while (getline(file, line)) {
            istringstream ss(line);
            string token;
            vector<string> tokens;
            while (getline(ss, token, ',')) {
                tokens.push_back(token);
            }
            if (tokens.size() < 5) continue;
            Timestamp ts = stoll(tokens[0]);
            double close = stod(tokens[4]);
            data[ts] = close;
        }
    }
    return data;
}

string timestamp_to_string(Timestamp ts) {
    time_t seconds = ts / 1000000;
    tm* ptm = gmtime(&seconds);
    char buffer[30];
    strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", ptm);
    return string(buffer);
}

int main() {
    double capital = 1000.0;
    double fee_btcusdt, fee_bnbbtc, fee_bnbusdt;

    cout << "Enter BTCUSDT fee (e.g., 0.001 for 0.1%): ";
    cin >> fee_btcusdt;
    cout << "Enter BNBBTC fee (e.g., 0.001): ";
    cin >> fee_bnbbtc;
    cout << "Enter BNBUSDT fee (e.g., 0.001): ";
    cin >> fee_bnbusdt;

    vector<string> btcusdt_files = {
        "BTCUSDT-1s-2025-07-11.csv",
        "BTCUSDT-1s-2025-07-12.csv",
        "BTCUSDT-1s-2025-07-13.csv",
        "BTCUSDT-1s-2025-07-14.csv"
    };

    vector<string> bnbbtc_files = {
        "BNBBTC-1s-2025-07-11.csv",
        "BNBBTC-1s-2025-07-12.csv",
        "BNBBTC-1s-2025-07-13.csv",
        "BNBBTC-1s-2025-07-14.csv"
    };

    vector<string> bnbusdt_files = {
        "BNBUSDT-1s-2025-07-11.csv",
        "BNBUSDT-1s-2025-07-12.csv",
        "BNBUSDT-1s-2025-07-13.csv",
        "BNBUSDT-1s-2025-07-14.csv"
    };

    auto btcusdt_data = load_data(btcusdt_files);
    auto bnbbtc_data = load_data(bnbbtc_files);
    auto bnbusdt_data = load_data(bnbusdt_files);

    ofstream outfile("result.csv");
    if (!outfile.is_open()) {
        cerr << "Failed to create result.csv" << endl;
        return 1;
    }

    double total_result = 0.0;
    size_t count = 0;

    // Объединяем по общим timestamp'ам
    for (const auto& [ts, btcusdt_close] : btcusdt_data) {
        if (bnbbtc_data.count(ts) && bnbusdt_data.count(ts)) {
            double bnbbtc_close = bnbbtc_data[ts];
            double bnbusdt_close = bnbusdt_data[ts];

            double result = (((capital
                            / btcusdt_close) * (1.0 - fee_btcusdt))
                            / bnbbtc_close) * (1.0 - fee_bnbbtc)
                            * (bnbusdt_close * (1.0 - fee_bnbusdt));

            total_result += result;
            ++count;

            outfile << timestamp_to_string(ts) << "," << fixed << setprecision(8) << result << "\n";
        }
    }

    outfile.close();

    // prepend итоговую строку
    ifstream in("result.csv");
    ofstream temp("temp.csv");
    temp << "Total:," << fixed << setprecision(8) << total_result << "\n";

    string line;
    while (getline(in, line)) {
        temp << line << "\n";
    }

    in.close();
    temp.close();
    remove("result.csv");
    rename("temp.csv", "result.csv");

    cout << "Finished. Total USDT result: " << fixed << setprecision(8) << total_result << endl;
    return 0;
}


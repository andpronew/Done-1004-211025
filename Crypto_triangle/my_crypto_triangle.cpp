#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <ctime>
#include <cmath>

using namespace std;

const double kCapital = 1000.0;
const float fee = 0.00075;
const int kClosePriceIndex = 4;   // индекс поля "Close"
const int kTimeIndex = 0;         // индекс поля "Open time" (в миллисекундах)

vector<string> split_line(const string& line, char delimiter = ',') {
    vector<string> result;
    stringstream ss(line);
    string token;

    while (getline(ss, token, delimiter)) {
        result.push_back(token);
    }

    return result;
}

// Преобразование миллисекунд в строку вида "YYYY-MM-DD HH:MM:SS"
string format_timestamp(const string& ms_str) {
    try {
        long long ms = stoll(ms_str);
        time_t sec = ms / 1000000;

        tm *utc_tm = gmtime(&sec); // UTC время

        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", utc_tm);

        return string(buffer);
    } catch (...) {
        return "InvalidTime";
    }
}

int main() {
    ifstream file_btc_usdt("BTCUSDT-1s-2025-07-14.csv");
    ifstream file_eth_btc("ETHBTC-1s-2025-07-14.csv");
    ifstream file_eth_usdt("ETHUSDT-1s-2025-07-14.csv");
    ofstream result_file("result.csv");

    if (!file_btc_usdt || !file_eth_btc || !file_eth_usdt || !result_file) {
        cerr << "Error: Could not open one or more files." << endl;
        return 1;
    }

    result_file << "datetime,result\n";

    string line1, line2, line3;
    size_t line_number = 0;

    while (getline(file_btc_usdt, line1) &&
           getline(file_eth_btc, line2) &&
           getline(file_eth_usdt, line3)) {

        line_number++;

        auto fields1 = split_line(line1);
        auto fields2 = split_line(line2);
        auto fields3 = split_line(line3);

        if (fields1.size() < 6 || fields2.size() < 6 || fields3.size() < 6) {
            cerr << "Error: Invalid line format at line " << line_number << endl;
            continue;
        }

        try {
            string timestamp_str = fields1[kTimeIndex];
            string datetime = format_timestamp(timestamp_str);

            double btc_usdt_close = stod(fields1[kClosePriceIndex]);
            double eth_btc_close = stod(fields2[kClosePriceIndex]);
            double eth_usdt_close = stod(fields3[kClosePriceIndex]);

            double result = kCapital * eth_usdt_close * pow((1-fee), 3)/ (btc_usdt_close * eth_btc_close);

            result_file << datetime << ',' << fixed << setprecision(8) << result << '\n';

        } catch (const exception& e) {
            cerr << "Error at line " << line_number << ": " << e.what() << endl;
        }
    }

    file_btc_usdt.close();
    file_eth_btc.close();
    file_eth_usdt.close();
    result_file.close();

    cout << "Computation completed. Output saved to result.csv" << endl;
    return 0;
}


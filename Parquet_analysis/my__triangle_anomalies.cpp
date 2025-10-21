// triangle_arbitrage_with_anomaly.cpp
// Совмещённый анализатор треугольного арбитража и аномального объёма торговли

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <tuple>
#include <ctime>
#include <iomanip>
#include <cmath>
#include <numeric>

using namespace std;

struct Candle {
    int64_t timestamp_ns;
    double price;
    double volume;
};

// Константы
constexpr size_t kWindowSize = 10;
constexpr double kAnomalyFactor = 2.0;
constexpr double kFeeRate = 0.00075;
constexpr double kCapital = 1000.0;

// Преобразование времени из наносекунд в строку
string format_time(int64_t timestamp_ns) {
    time_t sec = timestamp_ns / 1'000'000'000;
    tm *tm_time = gmtime(&sec);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_time);
    return string(buffer);
}

// Загрузка CSV-файла (без заголовков)
vector<Candle> load_csv(const string &filename) {
    vector<Candle> candles;
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        istringstream ss(line);
        string token;
        Candle candle;

        getline(ss, token, ',');
        candle.timestamp_ns = stoll(token);

        getline(ss, token, ',');
        candle.price = stod(token) / 1e8;

        getline(ss, token, ',');
        candle.volume = stod(token) / 1e8;

        candles.push_back(candle);
    }

    return candles;
}

// Обработка одной пары для аномалий
void process_pair_anomaly(const vector<Candle>& candles, const string& pair_name, ofstream& anomaly_log) {
    deque<double> volume_window;

    for (const auto& c : candles) {
        volume_window.push_back(c.volume);

        if (volume_window.size() > kWindowSize)
            volume_window.pop_front();

        double avg_volume = accumulate(volume_window.begin(), volume_window.end(), 0.0) / volume_window.size();

        if (volume_window.size() == kWindowSize && c.volume > avg_volume * kAnomalyFactor) {
            anomaly_log << format_time(c.timestamp_ns) << ','
                        << pair_name << ','
                        << fixed << setprecision(8) << c.volume << ','
                        << fixed << setprecision(8) << avg_volume << ','
                        << kAnomalyFactor << endl;
        }
    }
}

int main() {
    vector<Candle> btcusdt = load_csv("BTCUSDT_6.csv");
    vector<Candle> ethbtc  = load_csv("XTZBTC_6.csv");
    vector<Candle> ethusdt = load_csv("XTZUSDT_6.csv");

    ofstream anomaly_log("anomalies.csv");
    anomaly_log << "Time,Pair,CurrentVolume,AvgVolume,AnomalyFactor" << endl;

    process_pair_anomaly(btcusdt, "BTCUSDT", anomaly_log);
    process_pair_anomaly(ethbtc,  "XTZBTC", anomaly_log);
    process_pair_anomaly(ethusdt, "XTZUSDT", anomaly_log);

    anomaly_log.close();

    ofstream result("triangular_result.csv");
    result << "Time,BTCUSDT,ETHBTC,ETHUSDT,Profit_Direct,Profit_Reverse" << endl;

    size_t n = min({btcusdt.size(), ethbtc.size(), ethusdt.size()});
    for (size_t i = 0; i < n; ++i) {
        const auto& b = btcusdt[i];
        const auto& e = ethbtc[i];
        const auto& u = ethusdt[i];

        double usdt_to_btc = kCapital / b.price * (1 - kFeeRate);
        double btc_to_eth  = usdt_to_btc / e.price * (1 - kFeeRate);
        double eth_to_usdt = btc_to_eth * u.price * (1 - kFeeRate);
        double profit_direct = eth_to_usdt - kCapital;

        double usdt_to_eth = kCapital / u.price * (1 - kFeeRate);
        double eth_to_btc  = usdt_to_eth * e.price * (1 - kFeeRate);
        double btc_to_usdt = eth_to_btc * b.price * (1 - kFeeRate);
        double profit_reverse = btc_to_usdt - kCapital;

        result << format_time(b.timestamp_ns) << ','
               << fixed << setprecision(2) << b.price << ','
               << fixed << setprecision(8) << e.price << ','
               << fixed << setprecision(2) << u.price << ','
               << fixed << setprecision(6) << profit_direct << ','
               << fixed << setprecision(6) << profit_reverse << endl;
    }

    result.close();
    cout << "Analysis completed: anomalies.csv and triangular_result.csv generated." << endl;
    return 0;
}


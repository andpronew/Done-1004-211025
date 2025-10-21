#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <format>  // C++20
#include <cassert>

using namespace std;
using namespace std::chrono;

struct Tick {
    long long tm;
    double ask_price;
    double ask_volume;
    double bid_price;
    double bid_volume;
};

string format_time(long long ns_since_epoch) {
    sys_time<nanoseconds> tp{nanoseconds{ns_since_epoch}};
    auto seconds_part = floor<seconds>(tp);
    auto nanos_part = ns_since_epoch % 1'000'000'000;
    return format("{:%Y-%m-%d %H:%M:%S}.{:09}", seconds_part, nanos_part);
}

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
            t.tm = stoll(fields[0]);
            t.bid_price  = stod(fields[1]) / 1e8;
            t.bid_volume = stod(fields[2]) / 1e8;
            t.ask_price  = stod(fields[3]) / 1e8;
            t.ask_volume = stod(fields[4]) / 1e8;
            assert(t.ask_price > t.bid_price);
            data.push_back(t);
        } catch (...) {
            continue;
        }
    }

    return data;
}

multimap<long long, int> m;

int main() {
    double capital = 100.0;
    double fee = 0.00075;

    vector<Tick> btcusdt = read_tick_csv("BTCUSDT_6_test.csv");
    vector<Tick> xtzbtc  = read_tick_csv("XTZBTC_6_test.csv");
    vector<Tick> xtzusdt = read_tick_csv("XTZUSDT_6_test.csv");

    vector<vector<Tick>*> v = {&btcusdt, &xtzbtc, &xtzusdt};
    if (btcusdt.empty() || xtzbtc.empty() || xtzusdt.empty()) return 0;

    vector<int> pxIdx = {-1, -1, -1};
    vector<int> idx = {0, 0, 0};
    long long t = max({btcusdt[0].tm, xtzbtc[0].tm, xtzusdt[0].tm});
    for (int id = 0; id < 3; ++id) {
        int i = 0;
        for (; i < v[id]->size(); ++i) {
            if ((*v[id])[i].tm >= t) {
                idx[id] = i;
                break;
            }
        }
        pxIdx[id] = (i > 0) ? i - 1 : 0;
        m.emplace((*v[id])[idx[id]].tm, id);
    }

    ofstream fout("result.csv");
    fout << "Time,BTCUSDT_ask,XTZBTC_ask,XTZUSDT_bid,UsedUSDT_Dir,Profit_Dir,Limit_Dir,"
         << "XTZUSDT_ask,XTZBTC_bid,BTCUSDT_bid,UsedUSDT_Rev,Profit_Rev,Limit_Rev\n";
    fout << fixed << setprecision(7);

    double total_profit_direct = 0.0;
    double total_profit_reverse = 0.0;
    
    ofstream out("out.txt");
    
    for (;;) {
        auto [t, id] = *m.begin();
        pxIdx[id] = idx[id];

        const auto &b = btcusdt[pxIdx[0]];
        const auto &x = xtzbtc[pxIdx[1]];
        const auto &z = xtzusdt[pxIdx[2]];

        // --- Прямой путь ---
        out << "____Forward_______________" << endl;
        double max_btc = b.ask_volume;
        out << "b.ask_volume = " << b.ask_volume  << endl;
        double max_xtz = x.ask_volume;
        out << "x.ask_volume = " << x.ask_volume  << endl;
        double max_usdt_to_btc = max_btc * b.ask_price;
        out << "x.ask_price = " << x.ask_price  << endl;
        out << "z.bid_volume = " << z.bid_volume  << endl;
        out << "z.bid_price = " << z.bid_price  << endl;
        out << "max_usdt_to_btc = " << max_usdt_to_btc  << endl;
        double max_btc_to_xtz = max_xtz * x.ask_price;
        out << "max_btc_to_xtz = " << max_btc_to_xtz << endl;
        double max_xtz_to_usdt = z.bid_volume * z.bid_price;
        out << "max_xtz_to_usdt = " << max_xtz_to_usdt << endl;
        double max_xtz_from_btc_in_usdt = max_btc_to_xtz * z.bid_price; 
        out << "max_xtz_from_btc_in_usdt = " << max_xtz_from_btc_in_usdt << endl;
        double max_capital_direct = min({capital, max_usdt_to_btc, max_xtz_from_btc_in_usdt, max_xtz_to_usdt});
        out << "max_capital_direct = " << max_capital_direct << endl;
 
        double usdt_to_btc = max_capital_direct / b.ask_price * (1 - fee);
        out << "usdt_to_btc with fee = " << usdt_to_btc << endl;
        double btc_to_xtz  = usdt_to_btc / x.ask_price * (1 - fee);
        out << "btc_to_xtz with fee = " << btc_to_xtz << endl;
        double xtz_to_usdt = btc_to_xtz * z.bid_price * (1 - fee);
        out << "xtz_to_usdt with fee = " << xtz_to_usdt << endl;
        double profit_direct = xtz_to_usdt - max_capital_direct;
        out << "profit_direct = " << profit_direct << endl;


        string limit_reason_dir = "-";
        if (max_capital_direct < capital) {
            if (max_capital_direct == max_usdt_to_btc) limit_reason_dir = "BTC ask vol";
            else if (max_capital_direct == max_btc_to_xtz) limit_reason_dir = "XTZ ask vol";
                else if (max_capital_direct == max_xtz_to_usdt) limit_reason_dir = "XTZ/USDT bid vol";
        out << "limit_reason = " << limit_reason_dir << endl;
        }
        out << "___________________" << endl;

        // --- Обратный путь ---
        out << "____Reverse_______________" << endl;
        double max_xtz2 = z.ask_volume; // / 1e8;
        double max_btc2 = x.bid_volume; // / 1e8;
       //  double z_ask_price_sat =  z.ask_price / 1e8;
        //double x_bid_price_sat =  x.bid_price / 1e8;
        double max_usdt_to_xtz = max_xtz2 * z.ask_price;
       double max_xtz_to_btc = max_btc2 * x.bid_price;
       //max_xtz = x.ask_volume;
        // double max_xtz_to_btc = max_btc2 / x.bid_price;
       // max_btc_to_xtz = max_xtz * x.ask_price;
        double max_capital_reverse = min({capital, max_usdt_to_xtz, max_xtz_to_btc * b.ask_price});

        double usdt_to_xtz2 = max_capital_reverse / z.ask_price * (1 - fee);
        double xtz_to_btc2  = usdt_to_xtz2 * x.bid_price * (1 - fee);
        double btc_to_usdt2 = xtz_to_btc2 * b.bid_price * (1 - fee);
        double profit_reverse = btc_to_usdt2 - max_capital_reverse;


        string limit_reason_rev = "-";
        if (max_capital_reverse < capital) {
            if (max_capital_reverse == max_usdt_to_xtz) limit_reason_rev = "XTZ ask vol";
            else if (max_capital_reverse == max_xtz_to_btc * z.ask_price) limit_reason_rev = "BTC bid vol";
        }
        out << "z.ask_volume = " << z.ask_volume << endl;
        out << "x.bid_volume = " << x.bid_volume << endl;
        out << "b.ask_price = " << b.ask_price << endl;
        out << "max_usdt_to_xtz = " << max_usdt_to_xtz << endl;
        out << "max_xtz_to_btc = " << max_xtz_to_btc << endl;
        out << "max_xtz_to_btc in usdt = " << max_xtz_to_btc * b.ask_price << endl;
        out << "max_capital_reverse = " << max_capital_reverse << endl;
        out << "usdt_to_xtz2 = " << usdt_to_xtz2 << endl;
        out << "xtz_to_btc2 = " << xtz_to_btc2 << endl;
        out << "btc_to_usdt2 = " << btc_to_usdt2 << endl;
        out << "profit_reverse = " << profit_reverse << endl;
        out << "limit_reason = " << limit_reason_rev << endl;

        out << "___________________" << endl;
        
        if (profit_direct > 0 || profit_reverse > 0) {
            fout << format_time(b.tm) << ","
                 << b.ask_price << "," << x.ask_price << "," << z.bid_price << ","
                 << max_capital_direct << "," << profit_direct << "," << limit_reason_dir << ","
                 << z.ask_price << "," << x.bid_price << "," << b.bid_price << ","
                 << max_capital_reverse << "," << profit_reverse << "," << limit_reason_rev << "\n";

            if (profit_direct > 0)
                total_profit_direct += profit_direct;
            if (profit_reverse > 0)
                total_profit_reverse += profit_reverse;
        }

        ++idx[id];
        if (idx[id] >= v[id]->size()) break;
        m.erase(m.begin());
        m.emplace((*v[id])[idx[id]].tm, id);
    }

    out.close();
    fout.close();

    cout << "Saved results to result.csv\n";
    cout << "Total profit (direct):  " << total_profit_direct << " USDT\n";
    cout << "Total profit (reverse): " << total_profit_reverse << " USDT\n";

    return 0;
}


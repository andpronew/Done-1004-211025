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
    long long tm; // время в наносекундах
    double ask_price;       // цена ask в сатошах
    double ask_volume;      // объём ask в сатошах
    double bid_price;       // цена bid в сатошах
    double bid_volume;      // объём bid в сатошах
};

// Преобразование наносекунд в строку времени с точностью до наносекунды
string format_time(long long ns_since_epoch) {
    sys_time<nanoseconds> tp{nanoseconds{ns_since_epoch}};
    auto seconds_part = floor<seconds>(tp);
    auto nanos_part = ns_since_epoch % 1'000'000'000;
    return format("{:%Y-%m-%d %H:%M:%S}.{:09}", seconds_part, nanos_part);
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
            t.tm = stoll(fields[0]);
            t.bid_price    = stod(fields[1]) / 1e8;
            t.bid_volume   = stod(fields[2]) / 1e8;
            t.ask_price    = stod(fields[3]) / 1e8;
            t.ask_volume   = stod(fields[4]) / 1e8;
            assert( t.ask_price > t.bid_price );
            data.push_back(t);
        } catch (...) {
            continue;
        }
    }

    return data;
}

multimap<long long, int> m; //map<time, currentcyId>

int main() {
    double capital = 100.0;
  //double fee = 0.00075; // 0.075% комиссия на каждую операцию
    double fee = 0.00075;

    vector<Tick> btcusdt = read_tick_csv("BTCUSDT_6.csv");
    vector<Tick> xtzbtc  = read_tick_csv("XTZBTC_6.csv");
    vector<Tick> xtzusdt = read_tick_csv("XTZUSDT_6.csv");

    vector<vector<Tick>*> v = {&btcusdt, &xtzbtc, &xtzusdt};
    if( btcusdt.empty() || xtzbtc.empty() || xtzusdt.empty() )
        return 0;

    vector<int> pxIdx = {-1, -1, -1};
    vector<int> idx = {0, 0, 0};
    long long t = max(max(btcusdt[0].tm, xtzbtc[0].tm), xtzusdt[0].tm);
    for( int id=0; id<3; ++id )
    {
      int i=0;
      for( ; i < v[id]->size(); ++i )
      {
        if( (*v[id])[i].tm >= t )
        {
          idx[id] = i;
          break;
        }
      }
      pxIdx[id] = (i>0) ? i-1 : 0;
      m.emplace((*v[id])[idx[id]].tm, id);
    }

    ofstream fout("result.csv");
    fout << "Time,BTCUSDT,XTZBTC,XTZUSDT,Profit_Direct,Profit_Reverse\n";
    fout << fixed << setprecision(7);

    double total_profit_direct = 0.0;
    double total_profit_reverse = 0.0;

    for ( ;; )
    {
        auto[t, id] = *m.begin();
        pxIdx[id] = idx[id];

        const auto &b = btcusdt[pxIdx[0]];
        const auto &x = xtzbtc[pxIdx[1]];
        const auto &z = xtzusdt[pxIdx[2]];

//        cout<<"t: "<<t<<" btcusdt: "<<t-b.tm<< " xtzbtc: "<<t-x.tm<<" xtzusdt: "<< t-z.tm<<endl;
//        cout<<idx[0]<<"("<<pxIdx[0]<<") "<<idx[1]<<"("<<pxIdx[1]<<") "<<idx[2]<<"("<<pxIdx[2]<<")"<<endl;

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

        fout<<"t: "<<t<<" btcusdt: "<<(t-b.tm)*1e-6<< " xtzbtc: "<<(t-x.tm)*1e-6<<" xtzusdt: "<< (t-z.tm)*1e-6<<endl;
        if (profit_direct > 0 || profit_reverse > 0) {
            fout << format_time(b.tm) << ","
                 << b.bid_price << "," << x.bid_price << "," << z.bid_price << ","
                 << b.ask_price << "," << x.ask_price << "," << z.ask_price << ","
                 << profit_direct << "," << profit_reverse << "\n";

            if (profit_direct > 0 )
              total_profit_direct += profit_direct;

            if (profit_reverse > 0 )
              total_profit_reverse += profit_reverse;
        }

        ++idx[id];
        if( idx[id]>=v[id]->size() )
          break;
        m.erase(m.begin());
        m.emplace((*v[id])[idx[id]].tm, id);
    }

    fout.close();

    cout << "Saved results to result.csv" << endl;
    cout << "Total profit (direct):  " << total_profit_direct << " USDT" << endl;
    cout << "Total profit (reverse): " << total_profit_reverse << " USDT" << endl;

    return 0;
}


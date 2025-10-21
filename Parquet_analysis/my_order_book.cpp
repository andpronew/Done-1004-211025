#include <iostream>
#include <iomanip>
#include <queue>
#include <vector>

using namespace std;

const double maker_fee_rate = 0.00075;
const double taker_fee_rate = 0.00075;

struct Order
{
    double price;
    double qty;
};

struct Trader
{
   double btc_balance;
   double usdt_balance;
};

void print_balances (const string& name, const Trader& t)
{
cout << name << " balances: \n";
cout << "BTC: " << t.btc_balance << "\n";
cout << "USDT: " << t.usdt_balance << "\n";
}

void initialize_order_book (vector<Order>& asks, vector<Order>& bids)
{
    asks.push_back ({100100.0, 0.2});
    asks.push_back ({100120.0, 0.3});
    asks.push_back ({100150.0, 0.5});

    bids.push_back ({100000.0, 0.2});
    asks.push_back ({99980.0, 0.3});
    asks.push_back ({99950.0, 0.5});
}

int main()
{
    cout << fixed << setprecision(2);

    vector<Order> ask_orders;
    vector<Order> bid_orders;

    initialize_order_book (ask_orders, bid_orders);
    
    Trader maker = {1.0, 0.0};
    Trader taker = {0.0, 100000.0};

    double total_profit = 0.0;

    cout << "======== Initial balances ======\n";

    print_balances ("Maker", maker);
    print_balances ("Taker", taker);
    cout << "================================\n\n";

    for (auto& ask : ask_orders)
    {
        if (taker.usdt_balance <= 0 || ask.qty <=0) 
            break;
    

    double price = ask.price;
    double qty = min (ask.qty, taker.usdt_balance / (price * (1 + taker_fee_rate)));
    if (qty == 0)
        continue;

    double trade_value = qty *price;
    double taker_fee = trade_value * taker_fee_rate;
    double maker_fee = trade_value *maker_fee_rate;
    double cost_basis = 100000 *qty;

    taker.btc_balance += qty;
    taker.usdt_balance -= (trade_value + taker_fee);

    maker.btc_balance -= qty;
    maker.usdt_balance += (trade_value - maker_fee);

    double profit = trade_value - cost_basis - maker_fee;
    total_profit += profit;

    ask.qty -= qty;

    cout << "Taker bought " << qty << " BTC at " << price << " USDT\n";
    cout << "Trade value: " << trade_value << " USDT\n";
    cout << "Taker fee: " << taker_fee <<  "USDT\n";
    cout << "Maker fee: " << maker_fee <<  "USDT\n";
    cout << "Maker profit: " << profit << "USDT\n" << endl;
    }
    cout << "\n======== Final balances ======\n";
    
    print_balances ("Maker", maker);
    print_balances ("Taker", taker);

    cout << "\n Total maker profit: " << total_profit << "USDT\n";

return 0;
}





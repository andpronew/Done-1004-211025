#include <iostream>
#include <string>
#include <iomanip>

using namespace std;

const double maker_fee = 0.0001;
const double taker_fee = 0.0004;

enum class OrderType {BUY, SELL};
enum class Role {MAKER, TAKER};

struct Order
{
    OrderType type;
    double price;
    double qty;
};

struct Market
{
    double best_bid;
    double best_ask;
};

Role determine_role(const Order& order, const Market& market)
{
    if (order.type == OrderType::BUY)
    {
        if (order.price >= market.best_ask)
            return Role::TAKER;
        else 
            return Role::MAKER;
    }
    else
    {
        if (order.price <= market.best_bid)
            return Role::TAKER;
        else
            return Role::MAKER;
    }
}

double calculate_fee(const Order& order, Role role)
{
    double fee_rate;
    if (role == Role::MAKER) 
        fee_rate = maker_fee;
    else
        fee_rate = taker_fee;
    return order.price * order.qty * fee_rate;
}

void process_order(const Order& order, const Market& market)
{
Role role = determine_role(order, market);
double fee = calculate_fee(order, role);

cout << fixed << setprecision(2);
cout << "Order: " << ((order.type == OrderType::BUY) ? "BUY" : "SELL") << order.qty << " @ " << order.price << endl;
cout << "Role: " << ((role == Role::MAKER) ? "MAKER" : "TAKER") << endl;
cout << "Fee: $" << fee << "\n\n";
}

int main()
{
    Market market = {100.00, 100.10};

    Order order1 = {OrderType::BUY, 100.10, 1.5};
    Order order2 = {OrderType::BUY, 99.90, 1.5}; 
    Order order3 = {OrderType::SELL, 99.90, 1.0};
    Order order4 = {OrderType::SELL, 100.20, 1.0}; 

    process_order(order1, market);
    process_order(order2, market);
    process_order(order3, market);
    process_order(order4, market);

    return 0;
}

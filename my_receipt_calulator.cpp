#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <vector>
#include <thread>
#include <chrono>
#include <bitset>
#include <stack>

using namespace std;

int main()
{
    int N = 0;
    cout << "Enter a number of items in the receipt: ";
    cin >> N;
    cout << "Enter a price (e.g. 26.11) and quantity (integer) of each item: " << endl;
    vector<float> price(N);
    vector<int> qty(N);
    float sum_before = 0.00f;
    for (int i = 0; i < N; i++)
    {
        cin >> price[i] >> qty[i];
        sum_before += price[i] * qty[i]; // calculate subsum
    }

    float discount = 0.00f;
    float tax = 0.07f * sum_before;
    float sum_after = sum_before;
    if (sum_before >= 100.00f)      // apply discount if subsum >= 100
    {
        discount = 0.10f * sum_before; // 10% discount
        sum_after = sum_before - discount;        // apply  discount
        tax = 0.07f * sum_after;      // recalculate tax
    } 

    cout << "Sum_before: " << fixed << setprecision(2)
         << sum_before << endl
         << "Discount: " << discount << endl
         << "Tax: " << tax << endl
         << "Total: "
         << sum_after + tax << endl;

    return 0;
}

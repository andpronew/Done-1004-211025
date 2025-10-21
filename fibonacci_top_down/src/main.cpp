#include <iostream>
#include <vector>
#include "../include/fibonacci.h"

using namespace std;

int main()
{
    int n = 10;
    vector<int> memo(n + 1, -1);
    cout << "Top-down Fibonacci(10): " << fibonacci(n, memo) << endl;
    return 0;
}

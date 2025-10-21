#include "../include/fibonacci.h"
#include <stdexcept>

int fibonacci(int n, vector<int> &memo)
{
    if (n < 0)
        throw std::out_of_range("Negative input not allowed");
    if (n <= 1)
        return n;
    if (memo[n] != -1)
        return memo[n];
    return memo[n] = fibonacci(n - 1, memo) + fibonacci(n - 2, memo);
}

#include <iostream>
#include <vector>
using namespace std;

int knapsack(int capacity, const vector<int>& weights, const vector<int>& values) 
{
    int n = weights.size();
    vector<vector<int>> dp(n + 1, vector<int>(capacity + 1, 0));

    for (int i = 1; i <= n; ++i) 
    {
        for (int w = 0; w <= capacity; ++w) 
        {
            if (weights[i - 1] <= w) 
            {
            dp[i][w] = max(dp[i - 1][w], dp[i - 1][w - weights[i - 1]] 
                    + values[i - 1]);
            } else 
            {
                dp[i][w] = dp[i - 1][w];
            }
        }
    }
    return dp[n][capacity];
}

int main() 
{
    int capacity = 100;
    vector<int> weights = {10, 20, 30, 40};
    vector<int> values = {60, 100, 120, 240};

    int max_value = knapsack(capacity, weights, values);
    cout << "Maximum value in Knapsack = " << max_value << endl;

    return 0;
}
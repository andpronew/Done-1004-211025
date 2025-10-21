/*
rm -rf build
cmake -B build
cmake --build build
cd build
ctest

*/

#include "gtest/gtest.h"
#include "../include/fibonacci.h"
#include <vector>

using namespace std;

TEST(FibonacciTest, HandlesBaseCases)
{
    vector<int> memo(2, -1);
    EXPECT_EQ(fibonacci(0, memo), 0);
    EXPECT_EQ(fibonacci(1, memo), 1);
}

TEST(FibonacciTest, HandlesSmallNumbers)
{
    vector<int> memo(6, -1);
    EXPECT_EQ(fibonacci(5, memo), 5);
}

TEST(FibonacciTest, HandlesLargerNumbers)
{
    int n = 10;
    vector<int> memo(n + 1, -1);
    EXPECT_EQ(fibonacci(10, memo), 55);
    EXPECT_EQ(fibonacci(9, memo), 34);
    EXPECT_EQ(fibonacci(7, memo), 13);
}

TEST(FibonacciTest, HandlesNegativeInput)
{
    vector<int> memo(1, -1);
    EXPECT_THROW(fibonacci(-5, memo), invalid_argument);
}

TEST(FibonacciTest, ReusesMemoBetweenCalls)
{
    vector<int> memo(11, -1);
    EXPECT_EQ(fibonacci(10, memo), 55);
    EXPECT_EQ(memo[9], 34);            // check that result is stored
    EXPECT_EQ(fibonacci(9, memo), 34); // uses cached value
}

TEST(FibonacciTest, HandlesLargeInput)
{
    int n = 40;
    vector<int> memo(n + 1, -1);
    EXPECT_EQ(fibonacci(40, memo), 102334155);
}

TEST(FibonacciTest, HandlesTooSmallMemo)
{
    vector<int> memo(5, -1); // smaller than needed
    EXPECT_NO_THROW({
        int result = fibonacci(4, memo);
    });
}


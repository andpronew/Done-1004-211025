/*
rm -rf build
cmake -B build
cmake --build build
cd build
ctest

*/

#include "../include/fibonacci_down_top.h"
#include <gtest/gtest.h>

TEST(FibonacciBottomUpTest, HandlesBaseCases)
{
    EXPECT_EQ(fibonacci(0), 0);
    EXPECT_EQ(fibonacci(1), 1);
}

TEST(FibonacciBottomUpTest, HandlesSmallNumbers)
{
    EXPECT_EQ(fibonacci(5), 5);
    EXPECT_EQ(fibonacci(6), 7);
}

TEST(FibonacciBottomUpTest, HandlesLargerNumbers)
{
    EXPECT_EQ(fibonacci(10), 55);
    EXPECT_EQ(fibonacci(15), 610);
    EXPECT_EQ(fibonacci(20), 6765);
}

TEST(FibonacciBottomUpTest, HandlesInvalidInput)
{
    EXPECT_EQ(fibonacci(-5), -1);
}

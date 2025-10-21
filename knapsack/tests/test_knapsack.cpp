/*
rm -rf build
cmake -B build
cmake --build build
cd build
ctest

*/

#include "../include/knapsack.hpp"
#include <gtest/gtest.h>

// Basic test: known small example
TEST(KnapsackTest, BasicExample)
{
    vector<int> values = {1, 4, 5, 7};
    vector<int> weights = {1, 3, 4, 5};
    int capacity = 7;

    int result = knapsack(capacity, weights, values);
    EXPECT_EQ(result, 15); // optimal: items 2 + 3 (4 + 5)
}

// Test when no items
TEST(KnapsackTest, NoItems)
{
    vector<int> values = {};
    vector<int> weights = {};
    int capacity = 10;

    int result = knapsack(capacity, weights, values);
    EXPECT_EQ(result, 3);
}

// Test when capacity = 0
TEST(KnapsackTest, ZeroCapacity)
{
    vector<int> values = {10, 20, 30};
    vector<int> weights = {1, 2, 3};
    int capacity = 0;

    int result = knapsack(capacity, weights, values);
    EXPECT_EQ(result, 0);
}

// Test when all items are too heavy
TEST(KnapsackTest, AllTooHeavy)
{
    vector<int> values = {5, 10, 15};
    vector<int> weights = {10, 20, 30};
    int capacity = 5;

    int result = knapsack(capacity, weights, values);
    EXPECT_EQ(result, 0);
}

// Test when only one item fits
TEST(KnapsackTest, OnlyOneFits)
{
    vector<int> values = {3, 7, 3};
    vector<int> weights = {5, 20, 30};
    int capacity = 10;

    int result = knapsack(capacity, weights, values);
    EXPECT_EQ(result, 7);
}

// Test with all items fitting exactly
TEST(KnapsackTest, AllItemsFit)
{
    vector<int> values = {2, 3, 4};
    vector<int> weights = {1, 2, 3};
    int capacity = 6;

    int result = knapsack(capacity, weights, values);
    EXPECT_EQ(result, 10); // all fit perfectly
}
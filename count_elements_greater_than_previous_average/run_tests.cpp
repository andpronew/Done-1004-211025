#include <gtest/gtest.h>
#include "my_count_elements_greater_than_previous_average.hpp"

// Пример из условия
TEST(ResponseTimeTest, BasicCase)
{
    vector<int> v = {100, 200, 150, 300};
    EXPECT_EQ(count_above_average_days(v), 2);
}

// Пустой ввод
TEST(ResponseTimeTest, EmptyInput)
{
    vector<int> v = {};
    EXPECT_EQ(count_above_average_days(v), 0);
}

// Один элемент
TEST(ResponseTimeTest, SingleElement)
{
    vector<int> v = {100};
    EXPECT_EQ(count_above_average_days(v), 0);
}

// Все одинаковые значения
TEST(ResponseTimeTest, AllEqual)
{
    vector<int> v = {100, 100, 100, 100};
    EXPECT_EQ(count_above_average_days(v), 0);
}

// Постоянный рост
TEST(ResponseTimeTest, StrictlyIncreasing)
{
    vector<int> v = {10, 20, 30, 40, 50};
    // Каждый следующий больше среднего предыдущих → 4 случаев
    EXPECT_EQ(count_above_average_days(v), 4);
}

// Постоянное падение
TEST(ResponseTimeTest, StrictlyDecreasing)
{
    vector<int> v = {50, 40, 30, 20, 10};
    EXPECT_EQ(count_above_average_days(v), 0);
}

// Очень большие значения (проверка переполнения long long)
TEST(ResponseTimeTest, LargeValues)
{
    vector<int> v = {1000000000, 1000000000, 1000000000};
    EXPECT_EQ(count_above_average_days(v), 1);
}

// Проверка со случайными всплесками
TEST(ResponseTimeTest, FluctuatingValues)
{
    vector<int> v = {100, 50, 300, 200, 600, 100};
    // Анализ вручную:
    // i=1 avg=100 → 50>100? нет
    // i=2 avg=75  → 300>75? да (1)
    // i=3 avg=150 → 200>150? да (2)
    // i=4 avg=162.5 → 600>162.5? да (3)
    // i=5 avg=250 → 100>250? нет
    EXPECT_EQ(count_above_average_days(v), 4);
}
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
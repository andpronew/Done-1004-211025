#include "/mnt/big/study/budget_planner/include/budget_planner.hpp"
#include <gtest/gtest.h>

TEST(BudgetPlannerTest, TotalExpenses)
{
    BudgetPlanner planner(2000.0);
    planner.add_expense("Rent", 1000.0);
    planner.add_expense("Food", 500.0);
    EXPECT_DOUBLE_EQ(planner.total_expenses(), 1500.0);
}

TEST(BudgetPlannerTest, Savings)
{
    BudgetPlanner planner(3000.0);
    planner.add_expense("Rent", 1000.0);
    planner.add_expense("Utilities", 200.0);
    EXPECT_DOUBLE_EQ(planner.savings(), 1800.0);
}

TEST(BudgetPlannerTest, ExceedsIncome)
{
    BudgetPlanner planner(1000.0);
    planner.add_expense("Shopping", 1100.0);
    EXPECT_TRUE(planner.exceeds_income());
}

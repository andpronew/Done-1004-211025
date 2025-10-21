#include "/mnt/big/study/budget_planner/include/budget_planner.hpp"
#include <stdexcept>

using namespace std;

void BudgetPlanner::add_expense(const string &category, double amount)
{
    expenses_.emplace_back(category, amount);
}

double BudgetPlanner::total_expenses() const
{
    double sum = 0.0;
    for (const auto &e : expenses_)
        sum += e.second;
    return sum;
}

double BudgetPlanner::savings() const
{
    return income_ - total_expenses();
}

bool BudgetPlanner::exceeds_income() const
{
    return total_expenses() > income_;
}
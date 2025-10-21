#include "/mnt/big/study/budget_planner/include/budget_planner.hpp"
#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

int main()
{
    BudgetPlanner planner;

    double income;
    cout << "Enter monthly income: ";
    cin >> income;
    planner.set_income(income);

    int N;
    cout << "Enter number of expense categories: ";
    cin >> N;

    cout << "Enter category and amount: " << endl;
    for (int i = 0; i < N; ++i)
    {
        string category;
        double amount;
        cin >> category >> amount;
        planner.add_expense(category, amount);
    }

    double total = planner.total_expenses();
    cout << "-------------------------" << endl;
    cout << "Total expenses: " << fixed << setprecision(2) << total << endl;

    if (planner.exceeds_income())
        cout << "Warning: expenses exceed income!" << endl;
    else
        cout << "You can save " << fixed << setprecision(2) << planner.savings() << endl;

    return 0;
}

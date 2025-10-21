#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <vector>
#include <thread>
#include <chrono>
#include <bitset>
#include <stack>

using namespace std;

class BudgetPlanner
{
private:
    double income_ = 0.0;
    vector<pair<string, double>> expenses_;

public:
    BudgetPlanner() = default;

    // Конструктор с аргументом
    explicit BudgetPlanner(double income)
        : income_(income)
    {
    }

    void set_income(double income) { income_ = income; }
    void add_expense(const string &category, double amount);
    double total_expenses() const;
    double savings() const;
    bool exceeds_income() const;
};

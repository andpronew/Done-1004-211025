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
    double income_;
    vector<pair<string, double>> expenses_; // pair of category and amount

public:
    BudgetPlanner() : income_(0.0) {}

    void set_income(double income) 
    {
        income_ = income;
    }
    
    void add_expense(const string& category, double amount)
    {
        expenses_.push_back({category, amount});
    }

    double total_expenses() const
    {
        double sum = 0.0;
        for (const auto& e : expenses_)
        {
            sum += e.second;
        }
        return sum;
    }
    
    double savings() const
    {
        return (income_ - total_expenses());
    }

    bool exceeds_income() const
    {
        return (total_expenses() > income_);
    }
    
    void print_report() const
    {
        cout << "-------------------------" << endl;
        cout << left << setw(15) << "Category" << "Amount" << endl;
        cout << "-------------------------" << endl;
        for (const auto& e : expenses_)
        {
            cout << left << setw(15) << e.first << fixed << setprecision(2) << e.second << endl;
        }
        
        cout << "-------------------------" << endl;

        double total = total_expenses();
        cout << "Total expenses: " << fixed << setprecision(2) << total << endl; 
        
        if (exceeds_income()) 
        {            
            cout << "Warning: expenses exceed income!" << endl;
        }
        else
        {
            cout << "You can save " << fixed << setprecision(2) << savings() << endl;
        }
    }

};

int main()
{
    BudgetPlanner planner;
    
    
    double income = 0;
    cout << "Enter monthly income: ";
    cin >> income;
    planner.set_income(income);
    
    int N = 0;
    cout << "Enter number of expense categories: ";
    cin >> N;
    
    cout << "Enter category and amount: " << endl;
    
    for (int i = 0; i < N; i++)
    {
        string category;
        double amount;
        cin >> category >> amount;
        planner.add_expense(category, amount);
    }
        
        
    planner.print_report();

       return 0;
 }

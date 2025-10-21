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

int main()
{
    double budget = 0;
    cout << "Enter monthly income: ";
    cin >> budget;
    int N = 0;
    cout << "Enter number of expense categories: ";
    cin >> N;
    vector<pair<string, double>> expenses(N);
    cout << "Enter category and amount: " << endl;
    
    for (int i = 0; i < N; i++)
    {
        cin >> expenses[i].first >> expenses[i].second;
    }    
    cout << "-------------------------" << endl;
    cout << left << setw(15) << "Category" << "Amount" << endl;
    cout << "-------------------------" << endl;
    for (const auto& expense : expenses)
    {
        cout << left << setw(15) << expense.first << fixed << setprecision(2) << expense.second << endl;
    }
    
    cout << "-------------------------" << endl;

    double sum_expenses = 0.0;
    for (const auto& expense : expenses)
    {
        sum_expenses += expense.second;
    }
    if (sum_expenses >= budget) 
    cout << "Warning: expenses exceed income!" << endl;
    else
    {
        cout << "-------------------------" << endl;
        cout << "Total expenses: " << fixed << setprecision(2) << sum_expenses << endl; 
        cout << "You can save " << fixed << setprecision(2) << (budget - sum_expenses) << endl;
    }
    

       return 0;
 }

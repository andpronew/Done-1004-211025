#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>

using namespace std;

bool is_power_of_two(int n) 
{
    if (n <= 0) return false;
    return (n && !(n & (n-1)));
}

int main()
{
    unsigned int number;
    cout << "Input positive number: ";
    cin >> number;

    is_power_of_two(number) ? cout << number << " is a power of two." << endl : cout << number 
    << " is not a power of two." << endl;



    return 0;
}


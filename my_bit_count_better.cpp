#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>

using namespace std;

int bit_count_better(unsigned int n)
{
    n = n - ((n >> 1) & 0x55555555);
    n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
    n = (n + (n >> 4)) & 0x0F0F0F0F;
    n = n + (n >> 8);
    n = n + (n >> 16);
    return n & 0x3F;
}

int main()
{
    unsigned int number;
    cout << "Input positive number: ";
    cin >> number;
    cout << "Number of set bits: " << bit_count_better(number) << endl;




    return 0;
}


#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>

using namespace std;

int bit_count_naive(unsigned int n)
{
    int count = 0;
    while (n)
    {
        count += n & 1;
        n >>= 1;
    }
    return count;
}

int main()
{
    unsigned int number;
    cout << "Input positive number: ";
    cin >> number;
    cout << "Number of set bits: " << bit_count_naive(number) << endl;




    return 0;
}


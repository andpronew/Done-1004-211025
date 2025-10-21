#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <bitset>

using namespace std;

unsigned int reverse_bits(unsigned int n)
{
    n = (n >> 16) | (n << 16);
    n = ((n & 0xff00ff00) >> 8) | ((n & 0x00ff00ff) << 8);
    n = ((n & 0xf0f0f0f0) >> 4) | ((n & 0x0f0f0f0f) << 4);
    n = ((n & 0xcccccccc) >> 2) | ((n & 0x33333333) << 2);
    n = ((n & 0xaaaaaaaa) >> 1) | ((n & 0x55555555) << 1);
    return n;
}
int main()
{
    uint32_t n;
    cout << "Input number: ";
    cin >> n;

    uint32_t rev = reverse_bits(n);
    cout << "Original: " << bitset<32>(n) << endl;
    cout << "Reversed (bin): " << bitset<32>(rev) << endl;
    cout << "Reversed (dec): " << rev << endl;  



    return 0;
}


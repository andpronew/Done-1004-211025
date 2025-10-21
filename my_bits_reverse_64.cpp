#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <bitset>

using namespace std;

uint64_t reverse_bits(uint64_t n)
{
    n = (n >> 32) | (n << 32);
    n = ((n & 0xffff0000ffff0000ULL) >> 16) | ((n & 0x0000ffff0000ffffULL) << 16);
    n = ((n & 0xff00ff00ff00ff00ULL) >> 8) | ((n & 0x00ff00ff00ff00ffULL) << 8);
    n = ((n & 0xf0f0f0f0f0f0f0f0ULL) >> 4) | ((n & 0x0f0f0f0f0f0f0f0fULL) << 4);
    n = ((n & 0xccccccccccccccccULL) >> 2) | ((n & 0x3333333333333333ULL) << 2);
    n = ((n & 0xaaaaaaaaaaaaaaaaULL) >> 1) | ((n & 0x5555555555555555ULL) << 1);
    return n;
}
int main()
{
    uint64_t n;
    cout << "Input number: ";
    cin >> n;

    uint64_t rev = reverse_bits(n);
    cout << "Original: " << bitset<64>(n) << endl;
    cout << "Reversed (bin): " << bitset<64>(rev) << endl;
    cout << "Reversed (dec): " << rev << endl;  



    return 0;
}


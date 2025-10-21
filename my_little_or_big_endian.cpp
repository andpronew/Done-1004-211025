#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>


using namespace std;

 int is_little_endian()
{
    unsigned int x =1;
    return *reinterpret_cast<unsigned char*>(&x);
}

unsigned combine(unsigned x, unsigned y)
{ return (y & 0xFFFFFF00) | (x & 0x000000FF);}

unsigned replace_byte(unsigned x, int i, unsigned char b)
{
    unsigned shift = i*8;
    unsigned mask = 0xFFu << shift;
    return (x & ~mask) | static_cast<unsigned>(b) << shift;
}

int main()
{
    cout << hex << showbase;
    cout << "is_little_endian(): " << is_little_endian() << endl;
    
    unsigned x = 0x89ABCDEF;
    unsigned y = 0x76543210;

    cout << "combine 0x89ABCDEF, 0x76543210 = " << combine(x, y) << endl;
 
    cout << "replace_byte 0x12345678, 2, 0xAB = " << replace_byte(0x12345678, 2, 0xAB) << endl;
    cout << "replace_byte 0x12345678, 0, 0xAB = " << replace_byte(0x12345678, 0, 0xAB) << endl;

    return 0;
}


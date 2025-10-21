#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include<cstddef>

using namespace std;
using byte_pointer = unsigned char*;

void show_bytes(byte_pointer start, std::size_t len)
{
    for(std::size_t i=0; i<len; i++)
    {cout << ' ' << hex << setw(2) << setfill('0') <<  (static_cast<int>(start[i]) & 0xFF);}

    cout << dec << setfill(' ') << '\n'; 
}

void show_short(short x)
{  show_bytes(reinterpret_cast<byte_pointer>(&x), sizeof(short)); }

void show_int (int x)
{  show_bytes(reinterpret_cast<byte_pointer>(&x), sizeof(int)); }

void show_long (long x)
{  show_bytes(reinterpret_cast<byte_pointer>(&x), sizeof(long)); }

void show_float (float x)
{  show_bytes(reinterpret_cast<byte_pointer>(&x), sizeof(float)); }

 void show_double (double x)
{  show_bytes(reinterpret_cast<byte_pointer>(&x), sizeof(double)); }

 void show_pointer (void* x)
{  show_bytes(reinterpret_cast<byte_pointer>(&x), sizeof(void*)); }

int main()
{
    int a = 123;
    float b = 123.0f;
    short c = 123;
    long d = 123;
    double e = 123.0f;
    int* p = &a;

    show_int(a);
    show_float(b);
    show_short(c);
    show_long(d);
    show_double(e);
    show_pointer(p);



    return 0;
}


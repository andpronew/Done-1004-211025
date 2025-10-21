// Half_adder, Full_adder, 16-bits-adder, 16-bits-incrementer
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>
#include <cstdint>
#include <bitset>
#include <stdexcept>

using namespace std;

struct Half_adder
{
bool sum_h;
bool carry_h;
};

struct Full_adder
{
bool sum_f;
bool carry_f;
};


Half_adder half_ad(bool a_h, bool b_h)
{
Half_adder result_h;
result_h.sum_h = a_h ^ b_h;
result_h.carry_h = a_h & b_h;
return result_h;
}

Full_adder full_ad(bool a_f, bool b_f, bool carry_in)
{
Full_adder result_f;
result_f.sum_f = a_f ^ b_f ^ carry_in;
result_f.carry_f = (a_f & b_f) | (a_f & carry_in) | (b_f & carry_in);
return result_f;
}

uint16_t add_16bit(uint16_t a_16, uint16_t b_16, bool &carry_out)
{
uint16_t result=0;
bool carry = false;

for (int i=0; i<16; ++i)
{
bool bit_a = (a_16>>i) & 1;
bool bit_b = (b_16>>i) & 1;

Full_adder fa = full_ad(bit_a, bit_b, carry);
if (fa.sum_f)
{ result |= (1<<i);}
carry = fa.carry_f;
}

carry_out = carry;
return result;
}

uint16_t bin_str_to_uint16(const string& bin) 
{
    if (bin.size() != 16) throw invalid_argument("Only 16 bits allowed ");
    uint16_t result = 0;
    for (char c : bin) {
        result <<= 1;
        if (c == '1') result |= 1;
        else if (c != '0') throw invalid_argument("Only '0' and '1' allowed ");
    }
    return result;
}

uint16_t incr_16bit (uint16_t value, bool &carry_out)
{
    uint16_t result = 0;
    bool carry = true;

    for (int i=0; i<16; ++i)
    {
        bool bit = (value >> i) & 1;
        Half_adder ha = half_ad (bit, carry);
        if (ha.sum_h)
        {
            result |= (1 << i);
        }
        carry = ha.carry_h;
    }
    carry_out = carry;
    return result;
}

int main()
{
    int a_h=0; 
    int b_h=0;
    cout << "Enter two bits for Halfadder: ";
    cin >> a_h >> b_h;
    Half_adder ha = half_ad (a_h, b_h);     
    cout << "Halfadder: sum = " << ha.sum_h << ", carry = " << ha.carry_h << endl;

    int a_f=0, b_f=0, carry_in = 0; 
    cout << "\nEnter three bits for Fulladder: ";
    cin >> a_f >> b_f >> carry_in;
    Full_adder fa = full_ad (a_f, b_f, carry_in);
    cout << "Full_adder: sum = " << fa.sum_f << ", carry = " << fa.carry_f << endl;

    string bin_a = "0000000000000000", bin_b = "0000000000000000";
    cout << "\nEnter two 16-bits numbers for 16-bits-adder: ";                                                            
    cin >> bin_a >> bin_b;

    uint16_t a = bin_str_to_uint16(bin_a);
    uint16_t b = bin_str_to_uint16(bin_b);

    bool carry_add = false;
    uint16_t sum = add_16bit(a, b, carry_add);

    cout << bin_a << " + " << bin_b << " = " << bitset<16>(sum) << " (carry = " << carry_add << ")\n";

    string val_str = "0000000000000000";
    cout << "\nEnter one 16-bits number for 16-bits-incrementer: ";
    cin >> val_str;

    try 
    {
        uint16_t val = bin_str_to_uint16(val_str);

        bool carry_inc = false;
        uint16_t inc = incr_16bit(val, carry_inc);

        cout << "Incrementor: " << bitset<16>(inc) << " (carry = " << carry_inc << ")" << endl;
    }
    catch (const invalid_argument& e) 
    {
        cerr << "Mistake: " << e.what() << endl;
    }

    return 0;
}

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <vector>
#include <bitset>
#include <iomanip>

using namespace std;

class ProgramCounter
{private:
    uint32_t pc_;
    uint32_t reset_pc_;

public:
    ProgramCounter(uint32_t reset_value = 0)
    : pc_(reset_value), reset_pc_(reset_value) {}

uint32_t get_pc() const { return pc_; } 

void next(uint32_t step = 4) { pc_ += step; }

void jmp(uint32_t addr) { pc_ = addr; }

void reset() { pc_ = reset_pc_; }

void print_state() const { cout << "PC: 0x" << hex << setw(8) << setfill('0') << pc_ << " (" << dec << pc_ << ")" << endl; }

};

int main()
{
    ProgramCounter pc(0);

    pc.print_state();

    pc.next();
    pc.print_state();
    pc.next();
    pc.print_state();
    pc.next();
    pc.print_state();

    pc.jmp(100);
    pc.print_state();
    
    pc.next();
    pc.print_state();
    pc.next();
    pc.print_state();

    pc.reset();
    pc.print_state();


     return 0;
}


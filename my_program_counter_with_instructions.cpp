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

void next(uint32_t step = 1) { pc_ += step; }

void jmp(uint32_t addr) { pc_ = addr; }

void reset() { pc_ = reset_pc_; }

void print_state() const { cout << "PC: " << pc_ << endl; }

};

int main()
{
    vector<string> program = 
    {
        "NEXT",
        "NEXT",
        "JMP 5",
        "NEXT",
        "NEXT",
        "NEXT",
        "RESET",
        "NEXT",
        "HALT"
    };

    ProgramCounter pc(0);
bool running = true;

while(running)
{
    uint32_t addr = pc.get_pc();
    if(addr >= program.size())
    {
        cout << "PC out of bounds, halting." << endl;
        break;
    }
    string instr = program[addr];
    cout << "Executing instruction at address " << addr << ": " << instr << endl;
    if(instr == "NEXT")
    {
        pc.next();
    }
    else if(instr.rfind("JMP", 0) == 0)
    {
        stringstream ss(instr);
        string op;
        uint32_t target;
        ss >> op >> target;
        pc.jmp(target);
    }
    else if(instr == "RESET")
    {
        pc.reset();
    }
    else if(instr == "HALT")
    {
        cout << "HALT reached " << endl;
        running = false;
    }
    else
    {
        cout << "Unknown instruction, halting." << instr << endl;
        running = false;
    }
    pc.print_state();

}

     return 0;
}


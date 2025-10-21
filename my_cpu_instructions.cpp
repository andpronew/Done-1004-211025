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
    unsigned int width_;
    unsigned int max_value_;
    unsigned int pc_;
    
public:
    ProgramCounter(unsigned int width)
    : width_(width), max_value_((1 << width) - 1), pc_(0) {}
    
    void reset() { pc_ = 0; }
    
    void next() 
    { 
        if(pc_ < max_value_)
            pc_++; 
        else
            pc_ = 0; // wrap around
    }
    
    void jmp(unsigned int addr)
    {
        pc_ = addr & max_value_; // ensure within bounds
    }

    unsigned int get_pc() const { return pc_; }
};

class SimpleCPU
{
private:
    ProgramCounter pc_;
    vector<string> instrucion_memory_;
    vector<int> data_memory_;
    vector<int> registers_;

public:
    SimpleCPU(unsigned int pc_width, unsigned int data_size, unsigned int reg_count)
    : pc_(pc_width), data_memory_(data_size, 0), registers_(reg_count, 0) {}

    void load_program(const vector<string>& program)
    {
        instrucion_memory_ = program;
    }

    void run(unsigned int max_steps = 50)
    {
        unsigned int steps = 0;
        while(steps < max_steps && pc_.get_pc() < instrucion_memory_.size())
        {
            string instr = instrucion_memory_[pc_.get_pc()];
            execute(instr);
            steps++;
        }
    }

private:
    void execute(const string& instr)
    {
        cout << "[PC=" << pc_.get_pc() << "]" << instr << endl;
            stringstream ss(instr);
            string opcode;
            ss >> opcode;
            if(opcode == "RESET")
        {
            pc_.reset();
        }
            else if(opcode == "JMP")
        {
            unsigned int addr;
            ss >> addr;
            pc_.jmp(addr);
            return; // avoid pc_ increment
        }
            else if (opcode == "BEQ")
            {
                unsigned int r, a, addr;
                ss >> r >> a >> addr;
                if(registers_[r] == (int)a)
                {
                    pc_.jmp(addr);
                    return; // avoid pc_ increment
                }
                pc_.next();
            }
            
            else if(opcode == "STORE")
            {
                unsigned int r, a;
                ss >> r >> a;
                data_memory_[a] = registers_[r];
                pc_.next();
            }

            else if(opcode == "LOAD")
            {
                unsigned int r, a;
                ss >> r >> a;
                registers_[r] = data_memory_[a];
                pc_.next();
            }
            else if(opcode == "ADD")
            {
                unsigned int r, val;
                ss >> r >> val;
                registers_[r] += val;
                pc_.next();
            }
            else if(opcode == "SUB")
            {
                unsigned int r, val;
                ss >> r >> val;
                registers_[r] -= val;
                pc_.next();
            }
            else if(opcode == "DUMP")
            {
                unsigned int r;
                ss >> r;
                cout << "Register[" << r << "] = " << registers_[r] << endl;
                pc_.next();
            }
            else
            {
                pc_.next(); // unknown instruction, just move to next
            }            
    }

};

int main()
{
    SimpleCPU cpu(4, 16, 4); // 4-bit PC, 16 data memory, 4 registers
    vector<string> program = 
    {
        "ADD 0 5",     // Add 5 to register 0
        "STORE 0 0",   // Store register 0 into data_memory[0]
        "LOAD 1 0",    // Load data_memory[0] into register 1
        "ADD 1 3",      // Add 3 to register 1
        "DUMP 1",      // Output register 1        
        "BEQ 1 8 3",   // If register 1 == 8, jump to instruction at address 8        
        "SUB 1 1",     // Subtract 1 from register 1
        "JMP 9",       // Jump back to instruction at address 4
        "ADD 2 42",    // Add 42 to register 2
        "DUMP 2",      // Output register 2
    };

    cpu.load_program(program);
    cpu.run(20); // Run with a maximum of 20 steps

     return 0;
}


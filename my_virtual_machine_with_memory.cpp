#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <vector>
#include <thread>
#include <chrono>
#include <bitset>
#include <stack>
#include <stdexcept>
#include <unordered_map>

using namespace std;

enum class OpCode
{
    PUSH, POP,
    ADD, SUB, MUL, DIV,
    LOAD, STORE,
    MOV_REG,
    JMP, JZ, JNZ,
    PRINT,
    HALT
};
enum class Register
{
    R0, R1
};

struct Instruction
{
    OpCode op;
    int arg1 = 0; // Could be a register or immediate value
    int arg2 = 0; // Could be a register or immediate value
};

class VirtualMachine
{
public:
    VirtualMachine()
    {
        memory_.resize(256, 0);
        reg_["R0"] = 0;
        reg_["R1"] = 0;
    }    

void run(const vector<Instruction>& program)
    {
        size_t ip = 0; // Instruction pointer
        while (ip < program.size())
        {
            const auto& instr = program[ip];
        
            switch (instr.op)
            {
            case OpCode::PUSH:
                stack_.push(instr.arg1);
                break;

            case OpCode::POP:
                pop();
                break;

            case OpCode::ADD:
            {
                int b = pop();
                int a = pop();
                stack_.push(a + b);
                break;
            }
            case OpCode::SUB:
            {
                int b = pop();
                int a = pop();
                stack_.push(a - b);
                break;
            }

            case OpCode::MUL:
            {
                int b = pop();
                int a = pop();
                stack_.push(a * b);
                break;
            }
            
            case OpCode::DIV:
            {
                int b = pop();
                int a = pop();
                if (b == 0)
                {
                    throw runtime_error("Division by zero");
                }
                stack_.push(a / b);
                break;
            }

            case OpCode::LOAD:
                check_addr(instr.arg1);
                stack_.push(memory_[instr.arg1]);
                break;

            case OpCode::STORE:
                check_addr(instr.arg1);
                memory_[instr.arg1] = pop();
                break;

            case OpCode::MOV_REG:
                if (instr.arg1 == 0) reg_["R0"] = instr.arg2;
                else reg_["R1"] = instr.arg2;
                break;

            case OpCode::JMP:
                ip = instr.arg1; 
                continue;
            
            case OpCode::JZ:
            {
                if (pop() == 0) ip = instr.arg1;
                continue;
            }
            
            case OpCode::JNZ:
            {
                if (pop() != 0) ip = instr.arg1;
                continue;
            }
            break;

            case OpCode::PRINT:
                cout << "Result: " << pop() << endl;
                break;
            case OpCode::HALT:
                return;
            }
            ++ip;
        }
    }
    void dump_memory(int start = 0, int end = 16)
    {
        cout << "Memory (addresses " << start << " - " << end - 1 << ")====\n";
        for (int i = start; i < end; ++i)
        {
            cout << "[" << i << "]: " << memory_[i] << "\n";
        }
    }

private:
    stack<int> stack_;
    vector<int> memory_;
    unordered_map<string, int> reg_;
    
    int pop()
    {
    if (stack_.empty()) throw runtime_error("Stack underflow");
        int val = stack_.top();
        stack_.pop();
        return val;
    }
    void check_addr(int addr)
    {
        if (addr < 0 || addr >= memory_.size())
        throw runtime_error("Memory access out of bounds");
    }
    
};

int main()
{
    vector<Instruction> program = 
    {
        {OpCode::PUSH, 2},
        {OpCode::PUSH, 3},
        {OpCode::ADD, 0},
        {OpCode::PUSH, 4},
        {OpCode::MUL},
        {OpCode::STORE, 14}, // Store result in memory[10]
        {OpCode::LOAD, 14},  // Load result from memory[10]
        {OpCode::PRINT},
        {OpCode::HALT}
    };
    VirtualMachine vm;
    vm.run(program);
    vm.dump_memory(0, 16);
        return 0;
    }

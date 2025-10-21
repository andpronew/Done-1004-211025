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

using namespace std;

enum class OpCode
{
    PUSH,
    ADD,
    SUB,
    PRINT
};

struct Instruction
{
    OpCode op;
    int operand; // Only used for PUSH
};

class VirtualMachine
{
public:
    void run(const vector<Instruction>& bytecode )
    {
        for (const auto &instr : bytecode)
        {
            switch (instr.op)
            {
            case OpCode::PUSH:
                stack_.push(instr.operand);
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
            case OpCode::PRINT:
                cout << "Result: " << pop() << endl;
                break;
            }
        }
    }
private:
    stack<int> stack_;
    int pop()
    {
    if (stack_.empty())
    {        
    throw runtime_error("Stack underflow");
    }
        int val = stack_.top();
        stack_.pop();
        return val;
    }
};

int main()
{
    vector<Instruction> bytecode = 
    {
        {OpCode::PUSH, 10},
        {OpCode::PUSH, 20},
        {OpCode::ADD, 0},
        {OpCode::PUSH, 4},
        {OpCode::SUB, 0},
        {OpCode::PRINT, 0}
    };
    VirtualMachine vm;
    vm.run(bytecode);
        return 0;
    }

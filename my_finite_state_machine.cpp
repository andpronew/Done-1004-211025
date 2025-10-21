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

using namespace std;

enum class State
{
    FETCH,
    DECODE,
    EXECUTE,
    HALT
};

class SimpleCPU
{
public:
    SimpleCPU() : state_(State::FETCH), instruction_counter_(0) {} 

    /// @brief Run the finite state machine
    void run() 
    {
        while (state_ != State::HALT) /// @brief Main loop of the FSM
        {
            switch (state_)     
            {
            case State::FETCH:
                fetch_instruction();
                break;
            case State::DECODE: 
                decode_instrucion();
                break;
            case State::EXECUTE:
                execute_instruction();
                break;
            case State::HALT:
                break;
            }
        }
        cout << "CPU halted." << endl;
    }
private:
    State state_;
    int instruction_counter_;

    /// @brief  Fetch the next instruction
    void fetch_instruction()
    {
        cout << "[FETCH] Fetching instruction at address: " << instruction_counter_ << endl;
        // Simulate fetching an instruction
        this_thread::sleep_for(chrono::milliseconds(500));
        state_ = State::DECODE;
    }

    void decode_instrucion()
    {
        cout << "[DECODE] Decoding instruction at address: " << instruction_counter_ << endl;
        // Simulate decoding an instruction
        this_thread::sleep_for(chrono::milliseconds(500));
        state_ = State::EXECUTE;
    }

    void execute_instruction()
    {
        cout << "[EXECUTE] Executing instruction at address: " << instruction_counter_ << endl;
        // Simulate executing an instruction
        this_thread::sleep_for(chrono::milliseconds(500));
        instruction_counter_++;
        if (instruction_counter_ >= 3) // Arbitrary condition to halt after 3 instructions
        {
            state_ = State::HALT;
        }
        else
        {
            state_ = State::FETCH;
        }
    }
};

int main()
{
    SimpleCPU cpu;
    cpu.run();


    return 0;
}

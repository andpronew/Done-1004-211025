#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <vector>
#include <bitset>

using namespace std;

class Counter
{private:
    unsigned int width_;
    uint32_t max_value_;
    uint32_t q_;
    uint32_t d_;

    bool clr_;
    bool en_;
    bool u_d_;
    bool ld_;

public:
    Counter(unsigned int width)
        : width_(width), max_value_((1 << width) - 1), q_(0), d_(0),
          clr_(false), en_(false), u_d_(true), ld_(false) {}

    void set_clr(bool value) { clr_ = value; }
    void set_en(bool value) { en_ = value; }
    void set_u_d(bool value) { u_d_ = value; }
    void set_ld(bool value) { ld_ = value; }
    void set_d(uint32_t value) { d_ = value & max_value_; }

    uint32_t get_q() const { return q_; }

void clock_tick()
    {
        if (clr_)
        {
            q_ = 0;
            return;
        }
        else if (ld_)
        {
            q_ = d_;
            return;
        }
        else if (en_)
        {
            if (u_d_)
            {
                if (q_ < max_value_) q_++;
            } else 
                {
                    if (q_ > 0) q_--;
                }
        }
            
}

void print_state() const
    {
        string bin;
        for (int i = int(width_) - 1; i >= 0; --i)
            bin.push_back(((q_ >> i) & 1) ? '1' : '0');
        
        cout << "Counter state: q = " << q_ << " 0b" << bin << endl;
    }
};

int main()
{
    Counter counter(4);

    counter.set_d(5);
    counter.set_ld(true);
    counter.clock_tick();
    counter.set_ld(false);
    counter.print_state();

    counter.set_en(true);
    counter.set_u_d(true);
    for (int i = 0; i < 3; ++i)
    {
        counter.clock_tick();
        counter.print_state();
    }
    counter.set_u_d(false);
    for (int i = 0; i < 2; ++i)
    {
        counter.clock_tick();
        counter.print_state();
    }
    counter.set_clr(true);
    counter.clock_tick();
    counter.set_clr(false);
    counter.print_state();

    return 0;
}


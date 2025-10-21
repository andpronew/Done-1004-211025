#include<iostream>

using namespace std;

class DFlipFlop
{
    private:
        bool q_;
        bool clk_;

    public:
        DFlipFlop() : q_ (false), clk_ (false) {}
        void update (bool d, bool clk)
        {
            if (!clk_ && clk)
            {q_ = d;}
            clk_ = clk;
        }

        bool output() const
        {return q_;}
};

int main()
{
DFlipFlop dff;

bool d =1;
bool clk = 0;

cout << "Initial Q: " << dff.output() << "\n";

dff.update(d, clk);
cout << "After CLK=0, Q: " << dff.output() << "\n";

clk = 1;
dff.update(d, clk);
cout << "After rising CLK=1, Q: " << dff.output() << "\n";

d = 0;
dff.update(d, clk);
cout << "D=0, but no new rising edge, Q: " << dff.output() << "\n";

clk = 0;
dff.update(d, clk);
clk = 1;
dff.update(d, clk);
cout << "After new rising edge, Q: " << dff.output() << "\n";

return 0;
}

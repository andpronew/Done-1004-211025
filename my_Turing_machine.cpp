#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>
#include <vector>
#include <map>

using namespace std;

struct Transition
{
char write_symb;
int move;
string next_st;
};

class Turingmachine
{
vector<char> tape;
int head;
string state;
string halt_st;
map<pair<string, char>, Transition> transitions;

public:
Turingmachine (const string& init_tape, const string& start_st, const string& halt) : 
  tape(init_tape.begin(), init_tape.end()),
  head(0),
  state(start_st),
  halt_st(halt) 
  {
  tape.resize(1000,'_');
  }


  void add_transit(const string& current_st, char read_symb, const Transition& trans)
  {
  transitions[{current_st, read_symb}] = trans;
  }

  void step()
  {
  char current_symb = tape[head];
  auto key = make_pair(state, current_symb);
  if (transitions.count(key) == 0)
  {
  cout << "No rule for (" << state << ", '" << current_symb;
  state = halt_st;
  return;
  }

  const Transition& trans = transitions[key];
  tape[head] = trans.write_symb;
  head += trans.move;
  state = trans.next_st;
  }

void run(bool verbose = false)
{
while (state != halt_st)
{
if (verbose) print();
step();
}
print();
}

void print()
{
cout << "State: " << state << "\nTape: ";
for (int i=0; i<20; ++i)
cout << tape[i]; 
cout << "\n  ";
for (int i=0; i<head; ++i)
cout << ' '; 
cout << "^\n";
}
};

int main()
{
Turingmachine tm("111_", "start", "halt");

tm.add_transit ("start", '1', {'X', 1, "go_right"});
tm.add_transit ("start", '_', {'_', 1, "restore"});

tm.add_transit ("go_right", '1', {'1', 1, "go_right"});
tm.add_transit ("go_right", '_', {'_', 1, "go_end"});

tm.add_transit ("go_end", '1', {'1', 1, "go_end"});
tm.add_transit ("go_end", '_', {'1', -1, "go_back"});

tm.add_transit ("go_back", '1', {'1', -1, "go_back"});
tm.add_transit ("go_back", '_', {'_', -1, "go_back"});
tm.add_transit ("go_back", 'X', {'X', 1, "start"});

tm.add_transit ("restore", 'X', {'1', 1, "restore"});
tm.add_transit ("restore", '1', {'1', -1, "restore"});
tm.add_transit ("restore", '_', {'_', 0, "halt"});

tm.run(true);

return 0;
}

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>

using namespace std;

const int sz=4;

struct part
{
int modelnumber;
int partnumber;
float cost;
};

int main()
{
int n;
  part apart[sz];

for (n=0; n<sz; n++)
{
  cout << "Enter model number: ";
  cin >> apart[n].modelnumber;
cout << "Enter part number: ";
  cin >> apart[n].partnumber;
  cout << "Enter cost: ";
  cin >> apart[n].cost;
}

for (n=0; n<sz; n++)
{
  cout << "Model number: " << apart[n].modelnumber;
cout << "Part number: " << apart[n].partnumber;
  cout << "Cost: " << apart[n].cost << endl;
}

  return 0;
}


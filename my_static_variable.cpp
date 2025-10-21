#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>
#include <unistd.h>

using namespace std;

void counter()
{
  static int count=0;
  count++;
  cout<< "count = " << count << endl;
}

int main() 
{
  counter();
  counter();
  counter();

  return 0;
}


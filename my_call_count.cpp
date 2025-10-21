#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>
#include <unistd.h>

using namespace std;

void limited_action()
{
  static int call_count=0;
  if (call_count<4)
  {
  cout << "Function has worked " << call_count+1 << " times\n";
  call_count++;
  }
  else
  {
  cout << "Calling limit has been reached!\n";
  }
}

int main() 
{
  for (int i=0; i<5; ++i)
  {
  limited_action();
  }
  return 0;
}


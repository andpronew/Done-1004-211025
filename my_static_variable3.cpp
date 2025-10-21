#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>
#include <unistd.h>

using namespace std;

void demo()
{
  static int a=1;
  static int b=100;

  cout << "a = " << a << ", b = " << b << endl;

  a += 1;
  b -= 10;
}

int main() 
{
  demo();
  demo();
  demo();

  return 0;
}


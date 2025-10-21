#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>
#include <unistd.h>

using namespace std;

void repchar()
  {
  for (int j=0; j<30; j++)
    cout << '+';
  cout << endl;
  }

void repchar(char ch)
  {
  for (int j=0; j<50; j++)
    cout << ch;
  cout << endl;
  }

void repchar(char ch, int n)
  {for (int j=0; j<n; j++)
    cout << ch;
  cout << endl;
  }


int main() 
{
  repchar();
  repchar('=');
  repchar('+', 30);
  
  return 0;
}


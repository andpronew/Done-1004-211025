#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>
#include <unistd.h>

using namespace std;

int arr[5] = {1, 26, 30, 126, 200};

int& get_el(int ind)
{
return arr[ind];
}

int main() 
{
  get_el(3) = 100;

  for (int i=0; i<5; ++i)
  cout << arr[i] << " ";
  
  return 0;
}


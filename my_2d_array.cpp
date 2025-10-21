#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>
#include <unistd.h>

using namespace std;

const int districts = 3;
const int months = 6;

int main() 
{
  int d, m;
  double sales [districts][months];

  for (d=0; d<districts; d++)
    for (m=0; m<months; m++)
    {
    cout << "Enter sales for district " << d+1;
    cout << ", month " << m+1 << ": ";
    cin >> sales [d][m];
    }
  cout << "\n\n";
  cout << "                                     Month\n";
  cout << "           1           2           3";

   for (d=0; d<districts; d++)
   {
   cout << "\nDistrict " << d+1;
     for (m=0; m<months; m++)
    cout << setiosflags(ios::fixed)
    << setiosflags(ios::showpoint)
    << setprecision(2)
    << setw(10)
    << sales [d][m];
}


  return 0;
}


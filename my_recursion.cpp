// Recursion for factorial
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>
#include <unistd.h>
#include <boost/multiprecision/cpp_int.hpp>

using namespace std;
using namespace boost::multiprecision;

cpp_int factfunc (unsigned long n)
{
  cpp_int k=0;
  if (n>1)
  {
    k=n*factfunc(n-1);
    cout << k << endl;
    return k;
  }
  else
    return 1;
}

int main() 
{
  int n;
   cpp_int fact;

  cout << "Enter an integer: ";
  cin >> n;
  fact = factfunc (n);
  cout << "Factorial of " << n << "= " << fact << endl;

  return 0;
}


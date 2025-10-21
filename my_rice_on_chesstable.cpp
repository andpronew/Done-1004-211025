//Рис на шахматной доске
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <boost/multiprecision/cpp_int.hpp>

using namespace std;
using namespace boost::multiprecision;

int main() 
{
    int i=1;
    cpp_int grains;
    const int total = 64;
    cout << "Enter numbers of grains on the first square: ";
    cin >> grains;     
	 while (i <= total) {
		 cout << "Square # " << i << " Grains: " << grains << endl;
		 grains = 2*grains;
		 i++;
		}

    return 0;
}

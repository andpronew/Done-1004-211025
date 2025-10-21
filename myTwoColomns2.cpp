#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>


using namespace std;

int main()
{
long year1=1990, year2=1991, year3=1992, popul1=1000, popul2=2000, popul3=2500;
cout << setw(8) << "Year" << setw(15) << "Population" << endl
	<< setw(8) << year1 << setfill('.') << setw(15) << popul1 << endl
	<< setfill(' ') << setw(8) << year2 << setfill('.') << setw(15) << popul2 << endl
	<< setfill(' ') << setw(8) << year3 << setfill('.') << setw(15) << popul3 << endl;

return 0;

}



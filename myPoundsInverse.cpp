#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>


using namespace std;

int main()
{
int p, sh, pen; 
float decP;
float decfrac;
cout << "Decimal pounds = ";
cin >> decP;

p = static_cast<int>(decP);
decfrac = decP - p;
sh = 20*decfrac;
pen = 12*decfrac;
cout << "Old equivalent = " << p << "." << sh << "." << pen << endl;



return 0;

}



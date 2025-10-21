#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>


using namespace std;

int main()
{
int a, b, c, d, num, denom;
char slash;
cout  << "Enter first fraction: ";
cin >> a >> slash  >> b;
cout  << "Enter second fraction: ";
cin >> c >> slash  >> d;

num = a*d+b*c;
denom = b*d;
cout  << "Sum: " << num << "/" << denom << endl;


return 0;

}



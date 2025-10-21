#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <cctype>

using namespace std;

int main()
{
double dollar, pound, euro;
cout << "Sum in dollars = ";
cin >> dollar;
pound = dollar/1.487;
euro = dollar/1.12;

cout << "Sum in pounds = " << pound << endl;
cout << "Sum in euros = " << euro << endl;


return 0;

}



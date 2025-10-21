#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main()
{
cout<<"Enter first name and age\n";
string firstName;
double age;
int ageMonths;
cin>>firstName;
cin>>age;
ageMonths=12*age;
cout<<"Hello, "<<firstName<<"(age"<<ageMonths<<")\n";
}

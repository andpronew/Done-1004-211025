#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <cctype>

using namespace std;

int main()
{
char i;
cout << "Input letter: "; 
cin >> i;
if (islower(i)==0) {
cout << "Upper" << endl;
}
	else {
		cout << "Lower" << endl;
	}


return 0;

}



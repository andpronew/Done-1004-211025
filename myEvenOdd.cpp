#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>
#include <unistd.h>
#include <algorithm>


using namespace std;

int main() {

int a;
string e;
cout << "Input integer number: ";
cin >> a;
if (a%2 == 0) {
	e = "even";
	}
	else {
		e = "odd";
	}

cout << e << endl;

return 0;

}



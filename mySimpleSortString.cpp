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

string a, b, c;
char comma=',';
cout << "Input 3 any words a,b,c: ";
cin >> a >> b >> c;
string arr[] = {a, b, c};
sort(arr, arr +3);
cout << "Increasing words: " << endl;
	for (int i=0; i<3 ; ++i)
		cout << arr[i] << " " << endl;
	

return 0;

}



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

int a, b, c;
char comma=',';
cout << "Input 3 integer numbers a,b,c: ";
cin >> a >> comma >> b >> comma >> c;
int arr[] = {a, b, c};
sort(arr, arr +3);
cout << "Increasing numbers: " << endl;
	for (int i=0; i<3 ; ++i)
		cout << arr[i] << " " << endl;
	

return 0;

}



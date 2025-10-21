//Калькулятор для обыковенных дробей
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <algorithm>

using namespace std;
int main() 
{
    cout << "Enter two fractions and operation (+,-,*,/): ";
    int a, b, c, d;
    char sl1, sl2;
    char oper;
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cin >> a >> sl1 >> b >> oper >> c >> sl2 >> d;     
 
    switch (oper) {
	case '+': cout << a*d + b*c << "/" << b*d << endl; break;
	case '-': cout << a*d -b*c << "/" << b*d << endl; break;
	case '*': cout << a*c << "/" << b*d << endl; break;
	case '/': cout << a*d << "/" << b*c << endl;break;
	default: cout << "Not correct input" << endl;
		}	
    return 0;
}

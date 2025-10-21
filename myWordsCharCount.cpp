#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>
#include <unistd.h>

using namespace std;

char getch() {
    char buf = 0;
    termios old = {};
    tcgetattr(STDIN_FILENO, &old);
    termios newt = old;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    read(STDIN_FILENO, &buf, 1);
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    return buf;
}

char getche() {
    char c = getch();
    cout << c;
    return c;
}

int main()
{
int chcount=0;
int wdcount=1;
char ch;

cout<<"Enter a phrase: ";
cout.flush();
 
while ((ch = getche()) != '\n') {
        if (ch == ' ')
            wdcount++;
        else
            chcount++;
    }

    cout << "\nWords = " << wdcount << endl
    	<< "Letters = " << chcount << endl;


return 0;

}



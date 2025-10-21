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
int x=10, y=10;
char dir='c';

cout<<"Type Enter to  quit\n ";
 
while (dir != '\n')
	{
		cout << "\nYour location is " << x << "," << y;
		cout << "\nPress direction (n, s, e, w): ";
	dir = getche();
	if (dir == 'n')
		y--;
		else
			if (dir == 's')
                		y++;
                		else
					if (dir == 'e')
               					 x++;
                				else
							if (dir == 'w')
        						        x--;
	  }

return 0;

}



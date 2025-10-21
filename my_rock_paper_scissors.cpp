//Рис на шахматной доске
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
    int choice, count, i;
    cout << "Enter your choice (0-rock, 1-paper, 2-scissors): ";
    cin >> choice;
		
    	i = time(0);  
	count = i % 3;
	 switch (count) {
			case 0: cout << "Rock" << endl; 
				
				break;
		 	case 1: cout << "Paper" << endl; break;
			case 2: cout << "Scissors" << endl; break;
			}
	if (choice == count) {
				cout << "Draw" << endl;
				}
	if ((choice==0 && count==2) || (choice==1 && count==0) || (choice==2 && count==1)) {
				cout << "You win! Congratulations!" << endl;
				}
	if ((choice==0 && count==1) || (choice==1 && count==2) || (choice==2 && count==0)) {
                                cout << "You lost! Sorry!" << endl;
                                }

    return 0;
}

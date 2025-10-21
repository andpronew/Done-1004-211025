//demonstrates structures using playing cards
//Instead of structures we can use classes: they are similar: попробовать 
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace std;

const int clubs = 0;
const int diamonds = 1;
const int hearts = 2;
const int spades = 3;
const int jack = 11;
const int queen = 12;
const int king = 13;
const int ace = 14;

struct card 
{
	int number; 
	int suit;
};

int main() 
{ 

card temp, chosen, prize;
int position;

card card1 = {7, clubs};
card card2 = {jack, hearts};
card card3 = {ace, spades};
prize = card3;
temp = card3; card3 = card1; card1 = temp;
cout << "I’m swapping card 2 and card 3\n";

temp = card3; card3 = card2; card2 = temp;
cout << "I’m swapping card 1 and card 2\n";

temp = card2; card2 = card1; card1 = temp;
cout << "Now, where (1, 2, or 3) is the ace of spades? ";

cin >> position;
switch (position) { 
	case 1: chosen = card1; break;
	case 2: chosen = card2; break;
	case 3: chosen = card3; break;
	}
if(chosen.number == prize.number && chosen.suit == prize.suit) 
	cout << "That’s right! You win!\n";

else cout << "Sorry. You lose.\n";
return 0;
}

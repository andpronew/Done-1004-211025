//та же карточная игра, но с полной колодой и выбор карт случайный

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <ctime>

using namespace std;

const int clubs = 0;
const int diamonds = 1;
const int hearts = 2;
const int spades = 3;

const int jack = 11;
const int queen = 12;
const int king = 13;
const int ace = 14;

struct card {
    int number;
    int suit;
};

string card_to_string(const card& c) { //// Функция для отображения карты (вместо цифр "where (1, 2, or 3) is the " можно будет указывать название карты
    string value;
    if (c.number >= 2 && c.number <= 10)
        value = to_string(c.number);
    else if (c.number == jack)
        value = "Jack";
    else if (c.number == queen)
        value = "Queen";
    else if (c.number == king)
        value = "King";
    else if (c.number == ace)
        value = "Ace";

    string s;
    switch (c.suit) {
        case clubs: s = "Clubs"; break;
        case diamonds: s = "Diamonds"; break;
        case hearts: s = "Hearts"; break;
        case spades: s = "Spades"; break;
    }

    return value + " of " + s;
}

int main() {
    // генератор случайных чисел
    srand(time(0));
    mt19937 rng(rand());

    // Создаём колоду
    vector<card> deck;
    for (int suit = 0; suit < 4; ++suit) {
        for (int number = 2; number <= ace; ++number) {
            deck.push_back({number, suit});
        }
    }

    // Перемешиваем колоду
    shuffle(deck.begin(), deck.end(), rng);

    // Берём первые три карты
    card card1 = deck[0];
    card card2 = deck[1];
    card card3 = deck[2];

    // Выбираем одну случайную карту как приз
    int prize_index = rand() % 3;
    card prize;
	if (prize_index == 0)
    		prize = card1;
		else if (prize_index == 1)
    			prize = card2;
			else
    			prize = card3;

    // Перемешивания
    card temp;
    temp = card3; card3 = card1; card1 = temp;
    cout << "I’m swapping card 2 and card 3\n";

    temp = card3; card3 = card2; card2 = temp;
    cout << "I’m swapping card 1 and card 2\n";

    temp = card2; card2 = card1; card1 = temp;
    cout << "Now, where (1, 2, or 3) is the " << card_to_string(prize) << "? ";

    // Игрок выбирает позицию
    int position;
    cin >> position;

    card chosen;
    switch (position) {
        case 1: chosen = card1; break;
        case 2: chosen = card2; break;
        case 3: chosen = card3; break;
        default:
            cout << "Invalid choice.\n";
            return 1;
    }

    if (chosen.number == prize.number && chosen.suit == prize.suit)
        cout << "That’s right! You win!\n";
    else {
        cout << "Sorry. You lose.\n";
        cout << "You chose: " << card_to_string(chosen) << endl;
        cout << "Correct was: " << card_to_string(prize) << endl;
    }

    return 0;
}

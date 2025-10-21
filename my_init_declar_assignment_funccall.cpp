//Initializion, declartion, assignment, function call

#include <iostream>
using namespace std;

int get_value() {
    cout << "Функция get_value() вызвана" << endl;
    return 42;
}

int main() {
    // ИНИЦИАЛИЗАЦИЯ с вызовом функции
    cout << "=== Инициализация ===" << endl;
    int a = get_value(); // переменная создаётся и сразу получает значение

    // ПРИСВАИВАНИЕ
    cout << "\n=== Присваивание ===" << endl;
    int b;              // переменная объявлена без значения
    b = get_value();    // теперь ей присваивается значение

    // ПОВТОРНОЕ ПРИСВАИВАНИЕ
    cout << "\n=== Повторное присваивание ===" << endl;
    b = get_value();    // можно присваивать сколько угодно раз

    return 0;
}




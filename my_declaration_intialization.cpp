// Примеры обьвления и инициализации

#include <iostream>
using namespace std;

// Глобальная переменная (будет инициализирована 0 по умолчанию)
int global_var;

class MyClass {
    // Инициализация члена прямо в классе
    int member_var_1 = 10;
    int member_var_2{20}; // списковая инициализация

public:
    // Инициализация через список инициализации конструктора
    MyClass(int value) : member_var_1(value) {}

    void print_values() {
        cout << "Использование класса "<< "member_var_1 = " << member_var_1 << endl;
        cout <<"Использование класса " << "member_var_2 = " << member_var_2 << endl;
    }
};

int main() {
    // Неинициализированная локальная переменная (ОПАСНО)
    int a;
    cout << "Неинициализированная переменная a (мусор): " << a << endl;

    // Копирующая инициализация
    int b = 5;

    // Прямая инициализация
    int c(10);

    // Uniform-инициализация (безопасная)
    int d{15};

    // Глобальная переменная
    cout << "Глобальная переменная global_var: " << global_var << endl;

    // Использование класса
    MyClass obj(100);
    obj.print_values();

    return 0;
}


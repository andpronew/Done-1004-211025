//значения & и * в грамматике C++

#include <iostream>
using namespace std;

// Обычная функция
void multiply(int a, int b) {
    cout << "Произведение: " << a * b << endl;
}

// Функция с передачей по ссылке (reference)
void increment(int& ref) {
    ref++;
}

// Функция с передачей по указателю
void double_value(int* ptr) {
    *ptr *= 2;
}

int main() {
    int x = 5;

    // & как оператор получения адреса
    int* ptr = &x;

    // * как разыменование указателя
    cout << "Значение через указатель: " << *ptr << endl;

    // * как объявление указателя
    int* another_ptr = ptr;

    // & как объявление ссылки
    int& ref = x;

    // Вызов функции с ссылкой
    increment(ref);

    // Вызов функции с указателем
    double_value(ptr);

    // & как побитовая операция
    int bitwise = x & 3;

    // * как оператор умножения
    multiply(x, 3);

    // Указатель на функцию
    void (*func_ptr)(int, int) = multiply;
    func_ptr(2, 4); // вызов функции через указатель

    // Вывод
    cout << "x = " << x << ", bitwise = " << bitwise << endl;

    return 0;
}

//Forward declaration (предварительное объявление) — это объявление идентификатора (обычно класса или функции) до его полного определения. Оно позволяет использовать имя типа или функции до того, как компилятор увидит его реализацию.
//Когда это нужно:
//Чтобы разрешить циклические зависимости между классами.
//Чтобы ускорить компиляцию, когда полное определение не нужно.
//Чтобы не тащить лишние #include, особенно в заголовках.

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace std;

// Forward declaration класса B: инициируем его еще до его определения, т.к. используется в определении класса А
class B;

class A {
public:
    void greet_b(B* b);  // Мы еще не знаем, что такое B, но знаем, что будет класс B
};

class B {
public:
    void say_hello() {
        cout << "Hello from class B!" << endl;
    }
};

// Теперь определяем метод A::greet_b, используя полный тип B
void A::greet_b(B* b) {
    cout << "A is calling B: ";
    b->say_hello();
}

int main() {
    A a;
    B b;

    a.greet_b(&b);

    return 0;
}


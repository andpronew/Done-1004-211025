#include <iostream>
using namespace std;


class Base {
public:
    void show() {
        cout << "Base\n";
    }
};


class Derived1 : public Base {
public:
    void show() { cout << "Derived1\n"; }
};


class Derived2 : public Base {
public:
    void show() { cout << "Derived2\n"; }
};


int main() {
 Base b;     //экземпляр базового класса
    Derived1 d1;
    Derived2 d2; 
   
   // Указатели на базовый тип
    Base* pb = &b;
    Base* p1 = &d1;
    Base* p2 = &d2;

    // Вызов полиморфных методов
    pb->show();  // выведет "Base"
    p1->show();  // выведет "Derived1"
    p2->show();  // выведет "Derived2"
   b.show(); 
   d1.show();
   d2.show();
    return 0;
}


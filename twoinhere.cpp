#include <iostream>
using namespace std;

class Base {
public:
    virtual void show() { cout << "Base\n"; }
};

class Derived1 : public Base {
public:
    void show() override { cout << "Derived1\n"; }
};

class Derived2 : public Base {
public:
    void show() override { cout << "Derived2\n"; }
};


int main() {
 Base b;     //экземпляр базового класса
 b.show();   // Выведет "Base"
    
    Derived1 d1;
    Derived2 d2; 
    // Приведение указателей на d1 и d2 к указателям на Base
    Base* b1 = &d1;  // неявное приведение
    Base* b2 = static_cast<Base*>(&d2);  // явное приведение

    b1->show();  // Выведет "Derived1"
    b2->show();  // Выведет "Derived2"
    
    return 0;
}


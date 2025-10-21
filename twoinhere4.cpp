#include <iostream>
using namespace std;

class Base {
public:
	virtual void show() = 0; 
};

class Derived : public Base {
public:
    void show() override { cout << "Derived\n"; }
};



int main() {
    Derived d;
    Base* ptr = &d;
    ptr->show(); 
   
    return 0;
}


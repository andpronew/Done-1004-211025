#include <iostream>

using namespace std;

class Base {
public:
   virtual ~Base() { cout << "Base destructor\n"; }
};

class Derived : public Base {
public:
    ~Derived() { cout << "Derived destructor\n"; }
};

int main() {
    Base* ptr = new Derived();
    delete ptr;  

return 0;
}

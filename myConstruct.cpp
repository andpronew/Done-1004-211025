#include <iostream>

using namespace std;

class Base {
public:
    virtual void who() = 0;
    virtual ~Base() {}
};

class Derived : public Base {
public:
    void who() override { cout << "I am Derived\n"; }
};

Base* create() {
    return new Derived();  
};

int main() {
    Base* obj = new Derived;
    delete obj;

return 0;

}


#include <iostream>

class Animal {
public:
    void info() {
        std::cout << "This is an animal.\n";
    }

    virtual void speak() {
        std::cout << "Animal sound\n";
    }
};

class Dog : public Animal {
public:
    void info() {
        std::cout << "This is a dog.\n";
    }

    void speak() override {
        std::cout << "Woof!\n";
    }
};

int main() {
    Dog dog;
    Animal* ptr = &dog;

    std::cout << "Calling info(): ";
    ptr->info();     // вызовется Animal::info (НЕВИРТУАЛЬНАЯ)

    std::cout << "Calling speak(): ";
    ptr->speak();    // вызовется Dog::speak (ВИРТУАЛЬНАЯ)

    return 0;
}


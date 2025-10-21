#include <iostream>

class Animal {
public:
    virtual void speak() {
        std::cout << "Some animal sound\n";
    }
};

class Dog : public Animal {
public:
    void speak() override {
        std::cout << "Woof!\n";
    }
};

class Cat : public Animal {
public:
    void speak() override {
        std::cout << "Meow!\n";
    }
};

int main() {
    Dog d;
    Cat c;

    // Cast to base pointer (неявное приведение)
    Animal* a1 = &d;
    Animal* a2 = &c;

    // Полиморфный вызов: поведение зависит от фактического типа объекта
    a1->speak();  // Выведет: Woof!
    a2->speak();  // Выведет: Meow!

    return 0;
}


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
        std::cout << "Гав!\n";
    }
};

class Cat : public Animal {
public:
    void speak() override {
        std::cout << "Мяу!\n";
    }
};

class Cow : public Animal {
public:
    void speak() override {
        std::cout << "Му!\n";
    }
};

int main() {
    // Создаём объекты разных животных
    Dog dog;
    Cat cat;
    Cow cow;

    // Массив указателей на базовый класс
    Animal* animals[] = { &dog, &cat, &cow };

    // Проходим по массиву и вызываем speak() — сработает нужная версия
    for (int i = 0; i < 3; ++i) {
        animals[i]->speak();  // Полиморфный вызов
    }

    return 0;
}


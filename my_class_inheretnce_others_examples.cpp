/*Примеры: 
Transport — базовый класс, у него есть поле model и виртуальный метод move().
Car и Bicycle — производные классы, переопределяющие move().
new создаёт объекты динамически, а delete освобождает память.
Используется полиморфизм — вызов move() работает корректно для каждого типа транспорта.
Виртуальный деструктор нужен, чтобы при delete вызывались деструкторы потомков. */

#include <iostream>

using namespace std;

// Базовый класс "Transport"
class Transport {
public:
    string model;

    Transport(const string& model_name) : model(model_name) {}

    virtual void move() {
        cout << model << " движется..." << endl;
    }

    virtual ~Transport() {}
};

// Наследник: "Car"
class Car : public Transport {
public:
    int horsepower;

    Car(const string& model_name, int hp)
        : Transport(model_name), horsepower(hp) {}

    void move() override {
        cout << model << " едет на " << horsepower << " л.с." << endl;
    }
};

// Наследник: "Bicycle"
class Bicycle : public Transport {
public:
    Bicycle(const string& model_name)
        : Transport(model_name) {}

    void move() override {
        cout << model << " едет с помощью педалей" << endl;
    }
};

int main() {
    Transport* t1 = new Car("BMW M3", 430);
    Transport* t2 = new Bicycle("Giant Escape");

    t1->move();  // BMW M3 едет на 430 л.с.
    t2->move();  // Giant Escape едет с помощью педалей

    delete t1;
    delete t2;

    return 0;
}


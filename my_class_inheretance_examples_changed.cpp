/*Примеры:
Furniture — базовый класс, у него есть поле material  и виртуальный метод for_room().
Chair и Table — производные классы, переопределяющие for_room().
new создаёт объекты динамически, а delete освобождает память.
Используется полиморфизм — вызов for_room() работает корректно для каждого типа мебели.
Виртуальный деструктор нужен, чтобы при delete вызывались деструкторы потомков. */

#include <iostream>

using namespace std;

// Базовый класс "Furniture"
class Furniture {
public:
    string material;

    Furniture (const string& model_name) : material(model_name) {} //конструктор Furniture

    virtual void for_room() {
        cout << material << " сделано из..." << endl;
    }

    virtual ~Furniture() {}
};
// Наследник: "Chair"
class Chair : public Furniture {
public:
    int size;
 Chair (const string& model_name, int sz)
        : Furniture (model_name), size(sz) {}

    void for_room() override {
        cout << material << " размер " << size << " cм" << endl;
    }
};
// Наследник: "Table"
class Table : public Furniture {
public:
    Table(const string& model_name)
        : Furniture(model_name) {}

    void for_room() override {
        cout << material << " для кухни" << endl;
    }
};

int main() {
    Furniture* f1 = new Chair("For fat men", 126);
    Furniture* f2 = new Table("For heavy eater");

    f1->for_room();
    f2->for_room(); 

    delete f1;
    delete f2;
    return 0;
}



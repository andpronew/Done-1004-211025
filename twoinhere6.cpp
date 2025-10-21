#include <iostream>
#include <vector>

using namespace std;

class Base {
public:
    virtual void show() {  // обязательно virtual!
        cout << "Base\n";
    }

    virtual ~Base() {}  // виртуальный деструктор для корректного удаления
};

class Derived1 : public Base {
public:
    void show() {
        cout << "Derived1\n";
    }
};

class Derived2 : public Base {
public:
    void show() {
        cout << "Derived2\n";
    }
};

int main() {
    vector<Base*> objects;

    // В разнобой добавляем объекты в вектор
    for (int i = 0; i < 10; ++i) {
        if (i % 3 == 0)
            objects.push_back(new Base);
        else
		if (i % 3 == 1)
            objects.push_back(new Derived1);

		else
			 if (i % 3 == 2)
            objects.push_back(new Derived2);
	
    }

    // Цикл вызова show() — работает полиморфизм
    for (Base* obj : objects) {
        obj->show();
    }

    // Освобождение памяти
    for (Base* obj : objects) {
        delete obj;
    }

    return 0;
}

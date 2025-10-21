/*Пример с:
new / delete — выделение и освобождение динамической памяти;
constructor / destructor;
virtual функции и полиморфизм;
static_cast — безопасное приведение типов */


#include <iostream>
#include <string>

using namespace std;

// Базовый класс
class Company {
public:
    Company(const string& name) : name_(name) {
        cout << "Company constructor: " << name_ << endl;
    }

    virtual ~Company() {
        cout << "Company destructor: " << name_ << endl;
    }

    virtual void work() const {
        cout << name_ << " makes work." << endl;
    }

protected:
    string name_;
};

// Производный класс
class Employee : public Company {
public:
    Employee(const string& name, const string& job_title)
        : Company(name), job_title_(job_title) {
        cout << "Employee constructor: job title = " << job_title_ << endl;
    }

    ~Employee() override {
        cout << "Employee destructor: " << job_title_ << endl;
    }

    void work() const override {
        cout << name_ << " works. Job title: " << job_title_ << endl;
    }

    void hard_work() const {
        cout << name_ << " is hardworking!" << endl;
    }

private:
    string job_title_;
};

int main() {
    // Создаём объект Employee, но через указатель на Company (полиморфизм)
    Company* company_ptr = new Employee("Salesman", "HRman");

    // Вызов виртуальной функции: будет вызван Employee::work
    company_ptr->work();

    // Приведение типа от Company* к Employee* (т.к. точно знаем, что это Employee)
    Employee* employee_ptr = static_cast<Employee*>(company_ptr);

    // Вызов метода, доступного только в Employee
    employee_ptr->hard_work();

    // Удаляем объект (будет вызван деструктор Employee, затем Company)
    delete company_ptr;

    return 0;
}


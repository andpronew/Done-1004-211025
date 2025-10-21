#include <iostream>
#include <string>

using namespace std;

struct my_string : public string {
  using string::string;
  static int s_i;

  template<typename ...Args>
  my_string(Args&&... args)
  :
    string(forward<Args>(args)...),
    _i(++s_i) {

    cout<<"s "<<_i<<endl;
  }
  ~my_string() {
    cout<<"~s "<<_i<<endl;
  }

  int _i{};
};

int my_string::s_i = 0;

struct Base
{
  Base(my_string name)
  :
    _name(move(name))
  {
    cout<<"Base "<<_name<<endl;
  }

  virtual ~Base()
  {
    cout<<"~Base "<<_name<<endl;
  }

  virtual void f() {
    cout<<"f_base"<<endl;
  }
  virtual void f2() = 0;
  virtual void draw() {
  cout << "draw from Base" << endl;
}

  my_string _name {}; 
};

struct A : public Base
{
  A(string name)
  :
    Base(my_string("Base_" + name))
  {
    cout<<"A "<<_name<<endl;
  }

  ~A()
  {
    cout<<"~A "<<_name<<endl;
  }

  void f() override {
    cout<<"f_a"<<endl;
  }

  void f2() override {
    cout<<"f2_a"<<endl;
  }
};

struct B : public Base
{
  B(string name)
  :
  Base(my_string("Base_" + name))  
  {
    cout<<"B "<<_name<<endl;
  }
  ~B()
{
    cout<<"~B "<<_name<<endl;
  }

  void f() {
    cout<<"f_b"<<endl;
  }

  void f2() {
    cout<<"f2_b"<<endl;
  }
};

struct X1 {
};

struct X2 {
  virtual ~X2() = default;
};

int main()
{
  cout<<"sizeof(X1)="<<sizeof(X1)<<endl;
  cout<<"sizeof(X2)="<<sizeof(X2)<<endl;

 Base* obj[2];
  obj[0] = new A("a");
  obj[1] = new B("b");

  cout<<endl;

  for( Base* o : obj ) {
    o->f();
    o->f2();
    delete o;
    cout<<endl;
  }
  return 0;

}

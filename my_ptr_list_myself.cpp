#include <iostream>
#include <vector>
#include <list>
#include <typeinfo>
#include <cxxabi.h>

template <typename T>
std::string readable_type_name() {
  const char* name = typeid(T).name();
  int status = 0;
  char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
  std::string result = (status == 0) ? demangled : name;
  free(demangled);
  return result;
}

using namespace std;

template<typename T>
class List {
  struct ListNode {
    ListNode* prev{};
    ListNode* next{};
    T data{};
  };

  ListNode* _front{};
  ListNode* _back{};
  size_t _size{};

public:
  class iterator {
    friend class List;

    ListNode* _ptr{};

  public:
    bool operator!=(iterator it) {
      return _ptr!=it._ptr;
    }
    T& operator*() {
      return _ptr->data;
    }
    iterator& operator++() {
      _ptr = _ptr->next;
      return *this;
    }
  };

  ~List() 
// деструктор пользовательского контейнера List. Его задача — освободить всю динамически выделенную память, чтобы не было утечек
  {
    for( ListNode* p = _front; p!=nullptr; ) 
//Инициализация указателя: начинаем с начала списка.Цикл до конца: проходим по всем элементам, пока не дойдём до конца (nullptr)
    {
//запоминаем следующий узел до того, как удалим текущий.
      ListNode* next = p->next;
/* Удаляем текущий узел: освобождаем память, выделенную под ListNode. Зачем нужен next перед delete: Потому что после delete p память по адресу p больше невалидна. Если бы вы попытались потом сделать p->next, это было бы использованием удалённой памяти. 
Аналогия из реальной жизни:есть стопка документов, связанных скрепками. Вы хотите выкидывать каждый документ один за другим, но перед тем как выкинуть, вы смотрите, к какому документу ведёт следующая скрепка, иначе потеряете всю стопку.*/
      delete p;
      p = next;
    }
  }
/*Этот деструктор:
проходит по всем узлам списка от _front до конца,
корректно освобождает память каждого узла,
предотвращает утечки памяти,
написан безопасно: никаких обращений к удалённым данным. */

  iterator end() {
    iterator it;
    return it;
  }
    
  iterator begin() {
    iterator it;
    it._ptr = _front;
    return it;
  }

  size_t size() { return _size; }
  bool empty() { 
    return _size==0;
  }
  T& front() {
    return _front->data;
  }
  T& back() {
    return _back->data;
  }

  void insert(iterator it, T x) 
/*метод вставки нового элемента перед итератором it в пользовательский двусвязный список. 
ListNode — это структура узла с data, prev и next.
_front — указатель на начало списка.
_back — указатель на конец списка.
_size — текущий размер списка.
it._ptr — указатель итератора на конкретный узел. */

  {
    ListNode* p = new ListNode; //Выделяем динамически память под новый узел p
    p->data = x; //Записываем переданное значение x в поле data узла
    _size++; //Увеличиваем счётчик размера списка.
   
    p->next = it._ptr; //Новый узел p теперь указывает вперёд на узел, перед которым мы вставляем (it._ptr).
    if( it._ptr==nullptr ) //Если it указывает на конец списка (вставка в хвост)
    {
      p->prev = _back; //Новый узел будет продолжением текущего хвоста, т.е. связываем p с предыдущим "последним" узлом.
      _back = p; //Обновляем _back: теперь новый узел — это хвост списка
    }
    else //Иначе: вставка не в конец. 
    {
      p->prev = it._ptr->prev; //Новый узел должен указывать назад на предшественника it._ptr
      it._ptr->prev = p; //Узел, перед которым мы вставляем, теперь указывает назад на новый p
    }

    if( it._ptr==_front )
      _front = p; //Если вставка происходит в самое начало (перед _front), то p становится новым началом списка
    else
      p->prev->next = p; //Иначе: у предыдущего узла (относительно it) надо направить next на p
  }

  void push_back(T x) {
    insert(end(), x);
  }
};

template<typename Container>
void f() {
  Container l;
  l.push_back(1);
  l.push_back(2);
  l.push_back(3);
  typename Container::iterator it = l.begin();
  ++it;
  l.insert(it, 26);

  cout<<"size: "<<l.size()<<endl;

  cout<<readable_type_name<Container>()<<":";
  for( typename Container::iterator it = l.begin(); it!=l.end(); ++it )
    cout<<" "<<*it;
  cout<<endl;
}

int main() {
  f<List<int>>();
  f<std::list<int>>();
  f<std::vector<int>>();

  return 0;
}


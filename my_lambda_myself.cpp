/* демонстрация лямбда-функций, захвата переменных, сортировки с компараторами и возврата значений из лямбд.*/

#include<iostream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

int main() {
  string extra = "extra"; // Создаём строку extra со значением "extra" — позже она будет использоваться внутри лямбды
  auto cmp1=[](string s1, string s2) // Сравнение строк по алфавиту (обычная сортировка по возрастанию).
  {
    return s1<s2;
  };
  auto cmp2=[](string s1, string s2) // Сортировка по убыванию
  {
    return s1>s2;
  };
  auto cmp3=[](string s1, string s2)
  {
    if(s1.length()==s2.length())
    return s1<s2;
    return s1.length()<s2.length(); // Логика: сначала по длине строки, если длины равны — по алфавиту
  };

  bool dir{};
  auto cmp_dir=[&dir](string s1, string s2) //&dir — захват булевой (по умолчанию false) переменной по ссылке.(s1 > s2) ^ dir — "исключающее ИЛИ": если dir == true, направление сравнения меняется.Это позволяет одним компаратором делать и прямую, и обратную сортировку, просто меняя dir.
  {
    if(s1==s2)
    return false;
    return (s1>s2) != dir;
  };
  vector<string> v; // Объявление вектора строк v, изначально пустой
  dir = true;
  sort(v.begin(), v.end(), cmp_dir);
  dir = false;
  sort(v.begin(), v.end(), cmp_dir);

  sort(v.begin(), v.end(), cmp1); //Сортировка с тремя разными заранее определёнными лямбдами.
  sort(v.begin(), v.end(), cmp2);
  sort(v.begin(), v.end(), cmp3);
  sort(v.begin(), v.end(), //сортировка инлайн-лямбдой
    [](string s1, string s2){
      if(s1.length()==s2.length())
        return s1<s2;
      return s1.length()<s2.length();
    });


  auto f=[&extra](string s) //Захват extra по ссылке. Печатает Hi <имя>. Возвращает строку вида: extra + имя
  {
    cout<<"Hi "<<s<<endl;
    return extra+" "+s;
  };

  cout<<"before "<<extra<<endl;
  extra = "new extra";
  cout<<"after "<<extra<<endl;

  auto ret = f("Andrey"); //Вызов лямбды: Вызов функции f, печатает: Hi Andrey, Возвращает строку "new extra Andrey"
cout << "ret=" << ret << endl; //ret=new extra
}
/*Вот что демонстрирует код:
Захват переменных в лямбдах по ссылке.
Передача лямбды в sort как компаратора.
Универсальный компаратор cmp_dir, управляющий направлением сортировки.
*/


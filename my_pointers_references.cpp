/* - references cannot be reassigned, but pointers can be.
- references cannot be left "empty" (null), but pointers can be.
- both for references and pointers you can modify the value at the address.
- the necessity to check for nullptr before dereferencing a pointer.
- passing arguments by pointer and by reference in functions.
*/
#include <iostream>

using namespace std;

void update_qnt_ptr(int* qnt)
{
    if (qnt)
    { *qnt +=10; }
}

void update_qnt_ref(int& qnt)
    { qnt +=10; } 

int main()
{
    int stock = 100;
    int reserve = 50;

    int& stock_ref = stock;
    stock_ref +=5;
    cout << "Stock (through reference): " << stock << endl;

    stock_ref = reserve;
    cout << "Stock after an etempt to reassign the reference: " << stock << endl;

    int* stock_ptr = &stock;
    *stock_ptr +=5;
    cout << "Stock (through pointer): " << stock << endl;

    stock_ptr = &reserve; // Reassign the pointer to other warehouse              
    *stock_ptr +=5;
    cout << "Stock (through pointer) at the other warehouse: " << stock << endl;

    int* null_ptr = nullptr;
    if (null_ptr)
    {*null_ptr = 999;}
    else
    {cout << "null_ptr points nothing\n";}

    update_qnt_ptr (&stock);
    cout << "Stock after the function with poiner: " << stock << endl;
    update_qnt_ref (stock);
    cout << "Stock after the function with reference: " << stock << endl;

    return 0;
}


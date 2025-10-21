#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <bitset>
#include <stack>

using namespace std;

int main()
{
    vector<int> arr = {1, 2, 3, 4, 5};
    size_t i = 0;
    for (i = arr.size(); i-- > 0; )
    {
        cout << arr[i] << " ";
    }
    cout << endl;
    return 0;
}
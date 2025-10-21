#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

int main() {
    int n;
    cout << "Enter number of digits: ";
    cin >> n;

    vector<int> digits(n);
    cout << "Enter the digits:\n";
    for (int i = 0; i < n; ++i) {
        cin >> digits[i];
    }

    // Сортируем для корректной работы next_permutation
    sort(digits.begin(), digits.end());

    cout << "\nAll permutations:\n";
    do {
        for (int d : digits)
            cout << d;
        cout << endl;
    } while (next_permutation(digits.begin(), digits.end()));

    return 0;
}






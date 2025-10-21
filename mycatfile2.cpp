#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main() {
    string filename, keyword;

    cout << "Введите имя файла: ";
    cin >> filename;

    cout << "Введите слово для поиска: ";
    cin >> keyword;

    ifstream file(filename);
    string line;
    int lineNumber = 1;

    while (getline(file, line)) {
        if (line.find(keyword) != string::npos) {
            cout << lineNumber << ": " << line << endl;
        }
        ++lineNumber;
    }

    file.close();
    return 0;
}

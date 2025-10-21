#include <iostream>
#include <fstream>
#include <deque>
#include <string>

using namespace std;

int main() {
    string filename;
    int n;

    cout << "Введите имя файла: ";
    cin >> filename;

    cout << "Сколько последних строк вывести? ";
    cin >> n;

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Ошибка: не удалось открыть файл " << filename << endl;
        return 1;
    }

    deque<string> lastLines;
    string line;

    while (getline(file, line)) {
        lastLines.push_back(line);
        if (lastLines.size() > n) {
            lastLines.pop_front(); // убираем лишние строки спереди
        }
    }

    file.close();

    cout << "\nПоследние " << n << " строк(и):\n";
    for (const auto& l : lastLines) {
        cout << l << endl;
    }

    return 0;
}

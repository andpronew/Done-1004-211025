#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

const size_t BUFFER_SIZE = 4096; // Размер буфера для чтения

int main() {
    string filename;
    int n;

    cout << "Введите имя файла: ";
    cin >> filename;

    cout << "Сколько последних строк вывести? ";
    cin >> n;

    ifstream file(filename, ios::binary | ios::ate);
    if (!file.is_open()) {
        cerr << "Ошибка: не удалось открыть файл " << filename << endl;
        return 1;
    }

    streampos filesize = file.tellg();
    int linesFound = 0;
    string result = "";
    vector<string> lines;

    streampos pos = filesize;
    string buffer;
    buffer.reserve(BUFFER_SIZE);

    while (pos > 0 && linesFound <= n) {
        size_t readSize = (pos >= BUFFER_SIZE) ? BUFFER_SIZE : static_cast<size_t>(pos);
        pos -= readSize;
        file.seekg(pos);

        buffer.resize(readSize);
        file.read(&buffer[0], readSize);

        for (int i = readSize - 1; i >= 0; --i) {
            char c = buffer[i];
            if (c == '\n') {
                if (!result.empty()) {
                    reverse(result.begin(), result.end());
                    lines.push_back(result);
                    result.clear();
                    linesFound++;
                    if (linesFound > n) break;
                }
            } else {
                result += c;
            }
        }
    }

    // Добавляем последнюю строку, если файл начался, но строка осталась
    if (!result.empty() && linesFound <= n) {
        reverse(result.begin(), result.end());
        lines.push_back(result);
    }

    file.close();

    // Печатаем строки в правильном порядке
    for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
        cout << *it << endl;
    }

    return 0;
}



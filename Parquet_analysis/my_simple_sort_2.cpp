#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;

int main() {
    ifstream file_a("file_a.csv");
    ifstream file_b("file_b.csv");
    ofstream output("merged_sorted.csv");

    string line_a, line_b;
    bool has_a = bool(getline(file_a, line_a));
    bool has_b = bool(getline(file_b, line_b));

    while (has_a && has_b) {
        double val_a = stod(line_a);
        double val_b = stod(line_b);

        if (val_a <= val_b) {
            output << val_a << '\n';
            has_a = bool(getline(file_a, line_a));
        } else {
            output << val_b << '\n';
            has_b = bool(getline(file_b, line_b));
        }
    }

    // Оставшиеся строки
    while (has_a) {
        output << line_a << '\n';
        has_a = bool(getline(file_a, line_a));
    }

    while (has_b) {
        output << line_b << '\n';
        has_b = bool(getline(file_b, line_b));
    }

    file_a.close();
    file_b.close();
    output.close();

    cout << "Files merged successfully.\n";
    return 0;
}


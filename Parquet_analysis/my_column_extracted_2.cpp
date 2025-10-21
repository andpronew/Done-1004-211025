#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include<cstdint>

using namespace std;

// Проверка: строка состоит только из цифр
bool is_all_digits(const string& s) {
    for (char ch : s) {
        if (!isdigit(ch)) return false;
    }
    return !s.empty();
}

void extract_first_column(const string& input_filename, const string& output_filename) {
    ifstream infile(input_filename);
    ofstream outfile(output_filename);

    if (!infile.is_open() || !outfile.is_open()) {
        cerr << "Failed to open file." << endl;
        return;
    }

    string line;
    size_t line_number = 0;
    while (getline(infile, line)) {
        ++line_number;

        size_t pos = line.find(',');
        if (pos != string::npos) {
            string first_column = line.substr(0, pos);

            if (is_all_digits(first_column)) {
                try {
                    uint64_t timestamp = stoull(first_column); // проверка на валидное число
                    outfile << timestamp << '\n';
                } catch (const exception& e) {
                    cerr << "Line " << line_number << " skipped: invalid number '" << first_column << "'\n";
                }
            } else {
                cerr << "Line " << line_number << " skipped: non-numeric '" << first_column << "'\n";
            }
        } else {
            cerr << "Line " << line_number << " skipped: no comma found\n";
        }
    }

    infile.close();
    outfile.close();
    cout << "Done. Valid timestamps written to: " << output_filename << endl;
}

int main() {
    string input_file = "BTCUSDT_6.csv";         // замените на своё имя файла
    string output_file = "BTCUSDT_6_time.csv";   // имя для результата

    extract_first_column(input_file, output_file);
    return 0;
}


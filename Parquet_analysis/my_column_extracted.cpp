#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

void extract_first_column(const string& input_filename, const string& output_filename) {
    ifstream infile(input_filename);
    ofstream outfile(output_filename);
    
    if (!infile.is_open() || !outfile.is_open()) {
        cerr << "Failed to open file." << endl;
        return;
    }

    string line;
    while (getline(infile, line)) {
        size_t pos = line.find(',');
        if (pos != string::npos) {
            string first_column = line.substr(0, pos);
            outfile << first_column << '\n';
        }
    }

    infile.close();
    outfile.close();
}

int main() {
    string input_file = "BTCUSDT_6.csv";   // замените на своё имя файла
    string output_file = "BTCUSDT_6_time.csv";

    extract_first_column(input_file, output_file);

    cout << "Done. First column extracted to: " << output_file << endl;
    return 0;
}


#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <functional>

using namespace std;

using Row = vector<string>;
using Table = vector<Row>;

using CompareFunc = function<bool(const Row&, const Row&)>;

Table read_csv(const string& filename) {
    Table data;
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        Row row;
        stringstream ss(line);
        string cell;
        while (getline(ss, cell, ',')) {
            row.push_back(cell);
        }
        if (!row.empty())
            data.push_back(row);
    }
    return data;
}

void write_csv(const string& filename, const Table& data) {
    ofstream file(filename);
    for (const Row& row : data) {
        for (size_t i = 0; i < row.size(); ++i) {
            file << row[i];
            if (i + 1 < row.size())
                file << ",";
        }
        file << "\n";
    }
}

int partition(Table& data, int low, int high, CompareFunc comp) {
    Row pivot = data[high];
    int i = low - 1;
    for (int j = low; j < high; ++j) {
        if (comp(data[j], pivot)) {
            ++i;
            swap(data[i], data[j]);
        }
    }
    swap(data[i + 1], data[high]);
    return i + 1;
}

void quicksort(Table& data, int low, int high, CompareFunc comp) {
    if (low < high) {
        int pi = partition(data, low, high, comp);
        quicksort(data, low, pi - 1, comp);
        quicksort(data, pi + 1, high, comp);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <column_index_to_sort_by>" << endl;
        return 1;
    }

    int column_to_sort = atoi(argv[1]);

    Table data = read_csv("input.csv");
    if (data.empty()) {
        cerr << "Error: input.csv is empty or not found" << endl;
        return 1;
    }

    if (column_to_sort < 0 || column_to_sort >= static_cast<int>(data[0].size())) {
        cerr << "Error: column index out of bounds" << endl;
        return 1;
    }

    CompareFunc comp = [column_to_sort](const Row& a, const Row& b) {
        try {
            double val_a = stod(a[column_to_sort]);
            double val_b = stod(b[column_to_sort]);
            return val_a < val_b;
        } catch (...) {
            return a[column_to_sort] < b[column_to_sort];
        }
    };

    quicksort(data, 0, static_cast<int>(data.size()) - 1, comp);

    write_csv("output_quick.csv", data);

    cout << "Sorting completed by column " << column_to_sort << ". Check output_quick.csv" << endl;
    return 0;
}

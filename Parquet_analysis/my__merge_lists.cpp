#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

using namespace std;

using csv_row = vector<int64_t>;
using csv_data = vector<csv_row>;

csv_data read_csv(const string& filename) 
{
    csv_data data;
    ifstream file(filename);
    string line;

    while (getline(file, line)) 
    {
        stringstream ss(line);
        string cell;
        csv_row row;

        while (getline(ss, cell, ',')) 
        {
            row.push_back(stoll(cell));
        }
        data.push_back(row);
    }

    return data;
}

csv_data merge_sorted_csv(const csv_data& a, const csv_data& b) 
{
    csv_data result;
    size_t i = 0, j = 0;

    while (i < a.size() && j < b.size()) 
    {
        if (a[i][0] <= b[j][0]) 
        {
            result.push_back(a[i++]);
        } else 
        {result.push_back(b[j++]);}
    }

    while (i < a.size()) result.push_back(a[i++]);
    while (j < b.size()) result.push_back(b[j++]);

    return result;
}

void write_csv(const string& filename, const csv_data& data) 
{
    ofstream file(filename);
    for (const auto& row : data) 
    {
        for (size_t i = 0; i < row.size(); ++i) 
        {
            file << row[i];
            if (i + 1 < row.size()) file << ",";
        }
        file << "\n";
    }
}

int main() {
    // Пути к двум файлам
    string file_a = "sorted_a.csv";
    string file_b = "sorted_b.csv";

    // Чтение отсортированных данных
    csv_data a = read_csv(file_a);
    csv_data b = read_csv(file_b);

    // Слияние с сортировкой
    csv_data merged = merge_sorted_csv(a, b);

    // Запись результата
    write_csv("merged_sorted.csv", merged);

    cout << "Merged and sorted data written to merged_sorted.csv\n";
    return 0;
}


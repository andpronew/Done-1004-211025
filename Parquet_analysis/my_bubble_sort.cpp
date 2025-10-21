#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstdint>


using namespace std;

using csv_row = vector<double>;

vector<csv_row> read_csv(const string& filename) 
{
    vector<csv_row> data;
    ifstream file(filename);
    string line;

    while (getline(file, line)) 
    {
        stringstream ss(line);
        string cell;
        csv_row row;

        while (getline(ss, cell, ',')) 
        {
            row.push_back(stod(cell));
        }
        data.push_back(row);
    }

    return data;
}

vector<csv_row> merge_csv_files(const vector<string>& filenames) 
{
    vector<csv_row> merged;
    for (const auto& name : filenames) 
    {
        auto rows = read_csv(name);
        merged.insert(merged.end(), rows.begin(), rows.end());
    }
    return merged;
}

void simple_sort(vector<csv_row>& data, size_t column_index) 
{
    bool swapped;
    size_t n = data.size();

    for (size_t i = 0; i < n - 1; ++i) 
    {
        swapped = false;
        for (size_t j = 0; j < n - i - 1; ++j) 
        {
            if (data[j][column_index] > data[j + 1][column_index]) 
            {
                swap(data[j], data[j + 1]);
                swapped = true;
            }
        }
        if (!swapped)
            break;
    }
}

void write_csv(const string& filename, const vector<csv_row>& data) 
{
    ofstream file(filename);
    for (const auto& row : data) 
    {
        for (size_t i = 0; i < row.size(); ++i) 
        {
            file << row[i];
            if (i < row.size() - 1)
                file << ",";
        }
        file << "\n";
    }
}

int main() 
{
    vector<string> input_files = 
    {
        "BTCUSDT_6_time.csv",
        "XTZBTC_6_time.csv",
        "XTZUSDT_6_time.csv"
    };

    size_t sort_column_index = 0;

    vector<csv_row> all_data = merge_csv_files(input_files);

    simple_sort(all_data, sort_column_index);

    write_csv("sorted_bubble.csv", all_data);

    cout << "Done. Output has been written to sorted_bubble.csv" << endl;
    return 0;
}


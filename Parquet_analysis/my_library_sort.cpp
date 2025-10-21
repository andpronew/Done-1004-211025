#include <iostream>
#include <fstream>
#include <sstream> 
#include <string> 
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm> 

using namespace std;

using Row = vector<string>;
using Table = vector<Row>;

Table read_csv(const string& filename)
{
    Table data;
    ifstream file(filename);
    string line;

    while (getline (file, line))
    {
    Row row;
    stringstream ss (line);
    string cell;
    while (getline (ss, cell, ','))
    {
        row.push_back(cell);
    }
    if (!row.empty())
        data.push_back(row);

}
return data;
}

void write_csv (const string& filename, const Table& data)
{
    ofstream file (filename);
    for (const Row& row : data)
    {
        for (size_t i=0; i < row.size(); ++i)
        {
            file << row[i];
            if (i + 1 < row.size())
                file << ",";
        }
        file << endl;
    }
}

bool compare_rows (const Row& a, const Row& b, int col_index)
{
    try
    {
        double val_a = stod(a[col_index]);
        double val_b = stod(b[col_index]);
        return val_a < val_b;
    }
    catch (...)
    {
        return a[col_index] < b[col_index];
    }
}


int main(int argc, char* argv[])
{
    if (argc !=2)
    {
        cerr << "Usage: ./program <column_index_to_sort_by>" << endl;
        return 1;
    }

    int column_to_sort = atoi (argv[1]);

    Table data = read_csv ("input.csv");
    if (data.empty())
    {
        cerr << "input.csv is empty or not found" << endl;
        return 1;
    }


    if (column_to_sort < 0 || column_to_sort >= static_cast<int>(data[0].size()))
    {
        cerr << "Column index is out of bounds" << endl;
        return 1;
    }

    sort (data.begin(), data.end(), [column_to_sort](const Row& a, const Row& b)
    {
        return compare_rows (a, b, column_to_sort);
    });

    write_csv ("output.csv", data);

    cout << "Sorted by Column " << column_to_sort << " Output.csv has been written" << endl;

return 0;
}


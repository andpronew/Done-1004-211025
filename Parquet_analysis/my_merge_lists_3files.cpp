#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;

bool merge_two_sorted_files(const string& file1, const string& file2, const string& output_file) 
{
    ifstream in1(file1), in2(file2);
    ofstream out(output_file);

    string line1, line2;
    bool has1 = bool(getline(in1, line1));
    bool has2 = bool(getline(in2, line2));

    while (has1 && has2) 
    {
        long long val1 = stoll(line1);
        long long val2 = stoll(line2);

        if (val1 <= val2) 
        {
            out << val1 << '\n';
            has1 = bool(getline(in1, line1));
        } else 
        {
            out << val2 << '\n';
            has2 = bool(getline(in2, line2));
        }
    }

    while (has1) 
    {
        out << line1 << '\n';
        has1 = bool(getline(in1, line1));
    }

    while (has2) 
    {
        out << line2 << '\n';
        has2 = bool(getline(in2, line2));
    }

    in1.close();
    in2.close();
    out.close();
    return true;
}

int main() 
{
    const string file_a = "XTZBTC_6_time.csv";
    const string file_b = "XTZUSDT_6_time.csv";
    const string file_c = "BTCUSDT_6_time.csv";
    const string temp_file = "temp_merge.csv";
    const string final_output = "merged_sorted.csv";

    merge_two_sorted_files(file_a, file_b, temp_file);
    merge_two_sorted_files(temp_file, file_c, final_output); 

    cout << "Three files merged successfully into '" << final_output << "'.\n";
    return 0;
}

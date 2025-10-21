
#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include<cstdint>


using namespace std;

int merge_three_sorted_files(const string& file_a, const string& file_b, const string& file_c, const string& output_file)
{
    ifstream fa(file_a), fb(file_b), fc(file_c);
    ofstream fout(output_file);

    string line_a, line_b, line_c;
    bool has_a = static_cast<bool>(getline(fa, line_a));
    bool has_b = static_cast<bool>(getline(fb, line_b));
    bool has_c = static_cast<bool>(getline(fc, line_c));

     while (has_a || has_b || has_c)
    {
        uint64_t time_a = has_a ? stoull(line_a) : numeric_limits<uint64_t>::max();
        uint64_t time_b = has_b ? stoull(line_b) : numeric_limits<uint64_t>::max();
        uint64_t time_c = has_c ? stoull(line_c) : numeric_limits<uint64_t>::max();

        if (time_a <= time_b && time_a <= time_c)
        {
            fout << line_a << '\n';
            has_a = static_cast<bool>(getline(fa, line_a));
        }
        else if (time_b <= time_a && time_b <= time_c)
        {
            fout << line_b << '\n';
            has_b = static_cast<bool>(getline(fb, line_b));
        }
        else
        {
            fout << line_c << '\n';
            has_c = static_cast<bool>(getline(fc, line_c));
        }
    }

    return 0;
}

int main()
{
    string file_a = "BTCUSDT_6_time.csv";
    string file_b = "XTZUSDT_6_time.csv";
    string file_c = "XTZBTC_6_time.csv";
    string output_file = "merged_output.csv";

    merge_three_sorted_files(file_a, file_b, file_c, output_file);
    return 0;
}


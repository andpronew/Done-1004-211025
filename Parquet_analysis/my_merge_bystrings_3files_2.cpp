#include <iostream>
#include <fstream>
#include <string>
#include <optional>
#include <cstdint>
#include <limits>

using namespace std;

int merge_three_sorted_files(const string& file_a, const string& file_b, const string& file_c, const string& output_file) 
{
    ifstream fa(file_a);
    ifstream fb(file_b);
    ifstream fc(file_c);
    ofstream fout(output_file);

    auto read_line = [](ifstream& f) -> optional<string> 
    {
        string s;
        return getline(f, s) ? optional<string>{s} : nullopt;
    };

    auto get_time = [](const optional<string>& line) -> uint64_t 
    {return line ? stoull(*line) : numeric_limits<uint64_t>::max();};

    optional<string> line_a = read_line(fa);
    optional<string> line_b = read_line(fb);
    optional<string> line_c = read_line(fc);

    while (line_a || line_b || line_c) 
    {
        uint64_t time_a = get_time(line_a);
        uint64_t time_b = get_time(line_b);
        uint64_t time_c = get_time(line_c);

        if (time_a <= time_b && time_a <= time_c) 
        {
            fout << *line_a << "\n";
            line_a = read_line(fa);
        } else if (time_b <= time_a && time_b <= time_c) 
        {
            fout << *line_b << "\n";
            line_b = read_line(fb);
        } else 
        {
            fout << *line_c << "\n";
            line_c = read_line(fc);
        }
    }

    return 0;
}

int main() 
{
    const string file_a = "BTCUSDT_6_time.csv";
    const string file_b = "XTZUSDT_6_time.csv";
    const string file_c = "XTZBTC_6_time.csv";
    const string output_file = "merged_sorted_by_strings.csv";

    merge_three_sorted_files(file_a, file_b, file_c, output_file);

    cout << "Files merged successfully into '" << output_file << "'." << endl;
    return 0;
}



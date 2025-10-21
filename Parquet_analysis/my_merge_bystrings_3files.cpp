#include <iostream>
#include <fstream>
#include <string>
#include <optional>

using namespace std;

string extract_key(const string& line) 
{
    size_t pos = line.find(',');
    if (pos == string::npos) return line;
    return line.substr(0, pos);
}

bool key_less(const string& a, const string& b) 
{return a < b;}

int merge_three_sorted_files(const string& file_a, const string& file_b, const string& file_c, const string& output_file) 
{
    ifstream fa(file_a);
    ifstream fb(file_b);
    ifstream fc(file_c);
    ofstream fout(output_file);

    if (!fa.is_open() || !fb.is_open() || !fc.is_open() || !fout.is_open()) 
    {
        cerr << "Error opening files.\n";
        return 1;
    }

    optional<string> line_a, line_b, line_c;

    auto read_line = [](ifstream& f) -> optional<string> 
    {
        string s;
        if (getline(f, s)) return s;
        else return nullopt;
    };

    line_a = read_line(fa);
    line_b = read_line(fb);
    line_c = read_line(fc);

    while (line_a || line_b || line_c) 
    {
        struct KeyLine 
        {
            string key;
            string* line_ptr;
            int file_id;
        };

        KeyLine candidates[3];
        int count = 0;

        if (line_a) candidates[count++] = {extract_key(*line_a), &(*line_a), 0};
        if (line_b) candidates[count++] = {extract_key(*line_b), &(*line_b), 1};
        if (line_c) candidates[count++] = {extract_key(*line_c), &(*line_c), 2};

        int min_idx = 0;
        for (int i = 1; i < count; ++i) 
        {
            if (key_less(candidates[i].key, candidates[min_idx].key)) 
            {min_idx = i;}
        }

        fout << *(candidates[min_idx].line_ptr) << "\n";

        switch (candidates[min_idx].file_id) 
        {
            case 0: line_a = read_line(fa); break;
            case 1: line_b = read_line(fb); break;
            case 2: line_c = read_line(fc); break;
        }
    }

    return 0;
}

int main() 
{
    const string file_a = "XTZBTC_6_time.csv";
    const string file_b = "XTZUSDT_6_time.csv";
    const string file_c = "BTCUSDT_6_time.csv";
    const string output_file = "merged_sorted_by_strings.csv";

    merge_three_sorted_files(file_a, file_b, file_c, output_file);

    cout << "Files merged successfully into '" << output_file << "'." << endl;
    return 0;
}


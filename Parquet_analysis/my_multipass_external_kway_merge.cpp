#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <limits>
#include <cstdint>
#include <sstream>
#include <cstdio>  

using namespace std;

bool merge_k_files(const vector<string>& input_files, const string& output_file)
{
    size_t k = input_files.size();
    vector<ifstream> inputs(k);
    vector<uint64_t> current_values(k, numeric_limits<uint64_t>::max());
    vector<bool> has_value(k, false);

    string line;
    for (size_t i = 0; i < k; ++i)
    {
        inputs[i].open(input_files[i]);

        if (getline(inputs[i], line))
        {
            current_values[i] = stoull(line);
            has_value[i] = true;
        }
    }

    ofstream fout(output_file);

    while (true)
    {
        uint64_t min_val = numeric_limits<uint64_t>::max();
        int min_idx = -1;

        for (size_t i = 0; i < k; ++i)
        {
            if (has_value[i] && current_values[i] < min_val)
            {
                min_val = current_values[i];
                min_idx = i;
            }
        }

        if (min_idx == -1)
        {break;}

        fout << min_val << '\n';

        if (getline(inputs[min_idx], line))
        {
            current_values[min_idx] = stoull(line);
            has_value[min_idx] = true;
        }
        else
        {has_value[min_idx] = false;}
    }
    return true;
}

bool merge_files_multistep(const string& file_list, const string& final_output, size_t group_size)
{
    ifstream list(file_list);

    vector<string> all_files;
    string name;
    while (getline(list, name))
    {
        if (!name.empty())
        {all_files.push_back(name);}
    }

    size_t round = 0;
    vector<string> current_files = all_files;
    vector<string> temp_files;

    while (current_files.size() > 1)
    {
        temp_files.clear();
        for (size_t i = 0; i < current_files.size(); i += group_size)
        {
            vector<string> group;
            for (size_t j = i; j < i + group_size && j < current_files.size(); ++j)
            {group.push_back(current_files[j]);}

            stringstream ss;
            ss << "temp_merge_round" << round << "_group" << (i / group_size) << ".csv";
            string temp_output = ss.str();
            
            merge_k_files(group, temp_output);
            temp_files.push_back(temp_output);
        }

        if (round > 0)
        {
            for (const auto& f : current_files)
            {
                remove(f.c_str());
            }
        }

        current_files = temp_files;
        round++;
    }

    if (current_files.size() == 1)
    {rename(current_files[0].c_str(), final_output.c_str());}
        
    return true;
}

int main()
{
    string file_list = "file_list_for_merge.csv";
    string final_output = "merged_output.csv";
    size_t group_size = 50;

    merge_files_multistep(file_list, final_output, group_size);

    cout << "Merge completed successfully to merged_output.csv" << endl;
    return 0;
}


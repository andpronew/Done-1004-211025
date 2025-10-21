#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <map>

using namespace std;

bool merge_k_files(const vector<string>& input_files, const string& output_file)
{
    size_t k = input_files.size();
    vector<ifstream> inputs(k);
    multimap<uint64_t, size_t> value_to_index;

    for (size_t i = 0; i < k; ++i)
    {
        inputs[i].open(input_files[i]);
        uint64_t value;
        if (inputs[i] >> value)
        {value_to_index.insert({value, i});}
    }

    ofstream output(output_file);

    while (!value_to_index.empty())
    {
        auto it = value_to_index.begin();
        uint64_t value = it->first;
        size_t file_index = it->second;
        value_to_index.erase(it);

        output << value << '\n';

        uint64_t next_value;
        if (inputs[file_index] >> next_value)
        {value_to_index.insert({next_value, file_index});}
    }

    return true;
}

int main()
{
    string file_list = "file_list_for_merge.csv";
    string final_output = "merged_by_map_output.csv";

    ifstream list(file_list);
    vector<string> all_files;
    string name;

    while (getline(list, name))
    {
        if (!name.empty())
        { all_files.push_back(name); }
    }

    merge_k_files(all_files, final_output);

    cout << "Merge completed successfully to " << final_output << endl;
    return 0;
}


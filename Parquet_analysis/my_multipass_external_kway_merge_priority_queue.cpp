#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <queue>
#include <functional>

using namespace std;

bool merge_k_files(const vector<string>& input_files, const string& output_file)
{
    size_t k = input_files.size();
    vector<ifstream> inputs(k);

    for (size_t i = 0; i < k; ++i)
    {inputs[i].open(input_files[i]);}

    using Elem = pair<uint64_t, size_t>; 
    priority_queue<Elem, vector<Elem>, greater<Elem>> pq;

    for (size_t i = 0; i < k; ++i)
    {
        uint64_t value;
        if (inputs[i] >> value)
        {pq.push({value, i});}
    }

    ofstream output(output_file);

    while (!pq.empty())
    {
        auto [value, file_index] = pq.top();
        pq.pop();

        output << value << '\n';

        uint64_t next_value;
        if (inputs[file_index] >> next_value)
        {pq.push({next_value, file_index});}
    }

    return true;
}

int main()
{
    string file_list = "file_list_for_merge.csv";
    string final_output = "merged_output_priority_queue.csv";

    ifstream list(file_list);
    vector<string> all_files;
    string name;

    while (getline(list, name))
    {
        if (!name.empty())
        {all_files.push_back(name);}
    }

    merge_k_files(all_files, final_output);

    cout << "Merge completed successfully to " << final_output << endl;

    return 0;
}


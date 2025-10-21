#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;

int main() 
{
    ifstream file_a("XTZBTC_6_time.csv");
    ifstream file_b("XTZUSDT_6_time.csv");
    ofstream output("merged_sorted.csv");

     string line_a, line_b;   
    bool has_a = bool getline(file_a, line_a);
    bool has_b = bool getline(file_b, line_b);

    while (has_a && has_b) 
    {
        long long val_a = stoll(line_a);
        long long val_b = stoll(line_b);

        if (val_a <= val_b) 
        {
            output << val_a << '\n';
            has_a = bool(getline(file_a, line_a));
        } 
        else 
        {
            output << val_b << '\n';
            has_b = bool(getline(file_b, line_b));
        }
    }

    while (has_a) 
    {
        output << stoll(line_a) << '\n';
        has_a = bool(getline(file_a, line_a));
    }

    while (has_b) 
    {
        output << stoll(line_b) << '\n';
        has_b = bool(getline(file_b, line_b));
    }


    file_a.close();
    file_b.close();
    output.close();

    cout << "Files merged successfully.\n";
    return 0;
}


#include <iostream>
#include <vector>
#include "my_count_elements_greater_than_previous_average.hpp"

using namespace std;

int main()
{
    vector<int> values = {10, 20, 30, 40, 50};
    cout << "Count above average: " << count_above_average_days(values) << endl;
    return 0;
}

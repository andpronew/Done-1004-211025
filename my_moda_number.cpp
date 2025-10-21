//Нахождение моды ряда чисел (в т.ч. если моды две и более)
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace std;

int main()
{
int num;
vector<int> numbers;
cout<<"Enter numbers (Ctrl+D to finish): ";

while (cin>>num) {
	numbers.push_back(num);
	}

sort (numbers.begin(), numbers.end());

vector<int> modes;
int most_repeat = numbers[0];
int max_count = 1;
int current = numbers[0];
int current_count = 1;

for (int i=1; i<numbers.size(); ++i) {
	if (numbers[i]==current) {
		current_count++;
	}
	else {
		if (current_count > max_count)
			{
				max_count = current_count;
				modes.clear();
				modes.push_back(current);
            } else if (current_count == max_count) {
                modes.push_back(current);
            }
		current = numbers[i];
		current_count = 1;
	}
}
if (current_count > max_count) {
	max_count = current_count;
        most_repeat = current;	
	modes.clear();
        modes.push_back(current);
    } else if (current_count == max_count) {
        modes.push_back(current);
    }

cout << "Moda(s) = ";
for (int k : modes) {
		cout << k << " "; 
		}
		cout<< endl;
cout << "Number repeated = " << max_count << endl;
return 0;
}



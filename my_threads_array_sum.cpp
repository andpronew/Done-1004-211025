// Threads for array sum: array size data_size = 5000000, quantity of threads thread_count = 6
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <thread>
#include <vector>
#include <numeric>
#include <mutex>


using namespace std;

mutex result_mutex; // For protection of access to public var

void partial_sum(const vector<int>& data, size_t start, size_t end, long long& result) // The function for partial sum
{
long long local_sum = accumulate(data.begin() + start, data.begin() + end, 0LL);
lock_guard<mutex> lock(result_mutex); // protected access to public var
result += local_sum;
}

int main()
{
const size_t data_size = 5000000;
const size_t thread_count = 6;

vector<int> data(data_size); // fill numbers into the array
iota(data.begin(), data.end(), 1);

vector<thread> threads;
long long total_sum = 0;
size_t block_size = data_size / thread_count;

for (size_t i=0; i < thread_count; ++i)
{
size_t start = i * block_size;
size_t end;
if (i == thread_count - 1)  
end = data_size;
else end = start + block_size;

threads.emplace_back([&data, start, end, &total_sum]() 
{
partial_sum(data, start, end, total_sum);
}
);
}

for (auto& t : threads)
{t.join();}

cout << "Sum of all elements = " << total_sum << endl;

return 0;
}


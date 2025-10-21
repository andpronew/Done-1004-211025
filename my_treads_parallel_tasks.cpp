// Threads for parallel tasks
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <thread>

using namespace std;

void print_msg(const string& msg, int count) // The function will do threads 
{
for (int i=0;i < count; ++i)
{
cout << "Thread " << this_thread::get_id() << ": " << msg << " (" << i << ")\n";
}
}

int main()
{
thread thread1(print_msg, "Hi from the thread 1", 3); // Make two threads, using the same function with different parameters
thread thread2(print_msg, "Hi from the thread 2", 3);

thread1.join();
thread2.join();

cout << "The main thread has been finished!\n";

return 0;
}


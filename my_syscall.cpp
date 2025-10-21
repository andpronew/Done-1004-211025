// Syscall examples: open, read, close
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sys/syscall.h>
#include <fcntl.h>
#include <cstring>
#include <errno.h>



using namespace std;

int main() 
{
const char* filename = "test.txt";
const size_t buffer_size = 1024;
char buffer [buffer_size];

int fd = syscall (SYS_open, filename, O_RDONLY);
if (fd==-1)
{
cerr << "Mistake of the file openning: " << strerror(errno) << endl;
return 1;
}

ssize_t bytes_read;
while ((bytes_read = syscall (SYS_read, fd, buffer, buffer_size)) > 0)
{
cout.write (buffer, bytes_read);
}

if (bytes_read == -1)
{
cerr << "Mistake of the file reading: " << strerror(errno) << endl;
return 1;
}

if (syscall (SYS_close, fd) == -1)
{
cerr << "Mistake of the file closing: " << strerror(errno) << endl;
return 1;
}

return 0;
}


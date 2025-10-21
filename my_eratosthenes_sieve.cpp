//Решето Эратосфена
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <algorithm>

using namespace std;

int main() 
{
    int n;
    cout << "Enter last number of the list = ";
    cin >> n;
    vector<bool>primes (n+1, true);

    for (int i=2; i*i<=n; ++i) {
			if (primes[i]) {
				for (int j=i*i; j<=n; j+=i) {
					primes[j] = false;
					}
			}
	}
    cout << "Prime numbers: ";
    for (int i=2; i<=n; ++i) {
	    if (primes[i]) {
		    cout << i << " ";
	    }
    }
cout << endl;	
    return 0;
}

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <vector>
#include <list>
#include <chrono>

using namespace std;
using namespace chrono;

const int N = 50000;

void test_vector()
{
    vector<int> vec(N, 1);
    long long sum = 0;
    auto start = high_resolution_clock::now();
    for (int i = 0; i < N; ++i)
    sum += vec[i];
    auto end = high_resolution_clock::now();
    cout << "Vector sum: " << sum << "\n";
    cout << "Vector access time: "      
         << duration_cast<milliseconds>(end - start).count() << " ms" << endl;
    
}

void test_list()
{
    list<int> lst(N, 1);
    long long sum = 0;
    auto start = high_resolution_clock::now();
    for(int x : lst)
    sum += x;
    auto end = high_resolution_clock::now();
    cout << "List sum: " << sum << "\n";
    cout << "List access time: "      
         << duration_cast<milliseconds>(end - start).count() << " ms" << endl;

}
int matrix[N][N];
void row_wise_traversal(int matrix[N][N])
{
    long long sum = 0;
    auto start = high_resolution_clock::now();
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            sum += matrix[i][j];
    auto end = high_resolution_clock::now();
    cout << "Row-wise sum: " << sum << "\n";
    cout << "Row-wise traversal time: "      
         << duration_cast<milliseconds>(end - start).count() << " ms" << endl;
}

void column_wise_traversal(int matrix[N][N])
{
    long long sum = 0;
    auto start = high_resolution_clock::now();
    for (int j = 0; j < N; ++j)
        for (int i = 0; i < N; ++i)
            sum += matrix[i][j];
    auto end = high_resolution_clock::now();
    cout << "Column-wise sum: " << sum << "\n";
    cout << "Column-wise traversal time: "      
         << duration_cast<milliseconds>(end - start).count() << " ms" << endl;
}


int main()
{
    test_vector();
    test_list();

    for(int i = 0; i < N; ++i)
        for(int j = 0; j < N; ++j)
            matrix[i][j] = 1;
    
    row_wise_traversal(matrix);
    column_wise_traversal(matrix);

    return 0;
}


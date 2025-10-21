#include <iostream>
#include <unordered_map>
#include <tuple>
#include <algorithm>
#include <chrono>

using namespace std;
using namespace chrono;

long long call_count = 0; // number of tak()calls
int max_depth = 0;        // maximum recursion depth

// Key structure for memoization
struct Key
{
    int x, y, z;
    bool operator==(const Key &other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

// Hash function for Key
struct KeyHash
{
    size_t operator()(const Key &k) const noexcept
    {
        // Combination of hash values for x, y, z
        return ((size_t)k.x * 73856093u) ^ ((size_t)k.y * 19349663u) ^ ((size_t)k.z * 83492791u);
    }
};

int tak_plain(int x, int y, int z, int depth = 1)
{
    call_count++;
    max_depth = max(max_depth, depth);

    if (y >= x)
        return z;
    int a = tak_plain(x - 1, y, z, depth + 1);
    int b = tak_plain(y - 1, z, x, depth + 1);
    int c = tak_plain(z - 1, x, y, depth + 1);

    return tak_plain(a, b, c, depth + 1);
}

// Recursive tak function with memoization
int tak_memo(int x, int y, int z, unordered_map<Key, int, KeyHash> &memo, int depth = 1)
{
    call_count++;
    max_depth = max(max_depth, depth);

    Key key{x, y, z};
    auto it = memo.find(key);
    if (it != memo.end())
        return it->second;

    int result;
    if (y >= x)
    {
        result = z;
    }
    else
    {
        int a = tak_memo(x - 1, y, z, memo, depth + 1);
        int b = tak_memo(y - 1, z, x, memo, depth + 1);
        int c = tak_memo(z - 1, x, y, memo, depth + 1);
        result = tak_memo(a, b, c, memo, depth + 1);
    }

    memo[key] = result;
    return result;
}

template <typename Func>
void benchmark(const string &name, Func func)
{
    call_count = 0;
    max_depth = 0;

    auto start = high_resolution_clock::now();
    int result = func();
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();

    cout << "Benchmarking " << name << "...\n";
    cout << " result: " << result << "\n";
    cout << " calls: " << call_count << "\n";
    cout << " max depth: " << max_depth << "\n";
    cout << " time: " << duration << " microseconds\n\n";
}

// Test the tak function
int main()
{
    unordered_map<Key, int, KeyHash> memo;

    int x = 26, y = 15, z = 2;
    cout << "Count tak(" << x << "," << y << "," << z << ")...\n";

    benchmark("Plain Recursive", [&]()
              { return tak_plain(x, y, z); });
    benchmark("With Memoization", [&]()
              { return tak_memo(x, y, z, memo); });

    return 0;
}

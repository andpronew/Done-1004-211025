#include <iostream>
#include <unordered_map>
#include <tuple>
#include <algorithm>
using namespace std;

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

// Recursive tak function with memoization
int tak(int x, int y, int z, unordered_map<Key, int, KeyHash> &memo, int depth = 1)
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
        int a = tak(x - 1, y, z, memo, depth + 1);
        int b = tak(y - 1, z, x, memo, depth + 1);
        int c = tak(z - 1, x, y, memo, depth + 1);
        result = tak(a, b, c, memo, depth + 1);
    }

    memo[key] = result;
    return result;
}

// Test the tak function
int main()
{
    unordered_map<Key, int, KeyHash> memo;

    int x = 7, y = 5, z = 2;
    cout << "Count tak(" << x << "," << y << "," << z << ")...\n";

    int result = tak(x, y, z, memo);

    cout << "\nResult: " << result << "\n";
    cout << "Number of calls: " << call_count << "\n";
    cout << "Maximal depth of recursion: " << max_depth << "\n";
    cout << "Cache size: " << memo.size() << " elements\n";

    return 0;
}

#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <numeric>
#include <string>
#include <sstream>
#include <random>
#include <ctime>
 

using namespace std;

vector<string> read_words (const string& filename)
{
ifstream file (filename);
vector<string> words;
string word; 

while (file >> word)
{
words.push_back(word);
}
return words;
}

string get_random (const vector<string>& words, mt19937& rng)
{
    if (words.empty()) return "[empty]";
    uniform_int_distribution<size_t> dist (0, words.size() - 1);
    return words [dist(rng)];
}

int main()
{
mt19937 rng (static_cast<unsigned int> (time(nullptr)));

vector<string> words1 = read_words ("file1.txt");
vector<string> words2 = read_words ("file2.txt");
vector<string> words3 = read_words ("file3.txt");

string word1 = get_random (words1, rng);
string word2 = get_random (words2, rng);
string word3 = get_random (words3, rng);

cout << "Random sentence: " << word1 << " " << word2 << " " << word3 << endl;

return 0;
}


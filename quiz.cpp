#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <random>
#include <algorithm>
#include <cctype>
#include <limits>
#include <cstdlib>
#include <fstream>
#include <termios.h>
#include <unistd.h>

using namespace std;

struct Entry {
  char answer;
  string code;
};

vector<Entry> load_quiz(const string& filename) {
  vector<Entry> entries;
  ifstream file(filename);
  if (!file) {
    cerr << "Failed to open quiz file: " << filename << endl;
    exit(1);
  }
  string line;
  while (getline(file, line)) {
    if (line.size() < 3 || line[1] != '|') continue;
    char ch = toupper(line[0]);
    string code = line.substr(2);
    entries.push_back({ch, code});
  }
  return entries;
}

void clear_screen() {
  cout << "\033[2J\033[1;1H";
}

char get_single_char() {
  termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  char ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return ch;
}

void print_secret_ascii() {
  cout<<R"(
              ., _
             /    `
            ((|)))))
            ((/ a a
            )))   >)
           ((((._e((
          ,--/ (-.
         / \ <\/>/|
        / /)  Lo )|
       / / )    / |
      |   /    ( /
      |  /     ;/
      ||(      |
     / )|/|    \
    |/'/\ \_____\
         \   |  \
          \  |\  \
          |  | )  )
          )  )/  /
         /  /   /
        /   |  /
       /    | /
      /     ||
     /      ||
      '-,_  |_\
        ( '"'-`
gpyy     \(\_\
)";
}

string explain(char ch) {
  switch (ch) {
    case 'A': return "Initialization";
    case 'B': return "Function call";
    case 'C': return "Lambda/auto";
    case 'D': return "Loop or condition";
    case 'E': return "Class/template declaration";
    case 'F': return "Templates/types";
    case 'G': return "Input/output";
    case 'H': return "Exception handling";
    case 'I': return "Threads/mutexes";
    case 'J': return "STL structures";
    default: return "";
  }
}

void print_key_legend() {
  cout << "\033[90m";
  cout << "A: Initialization        B: Function call          C: Lambda/auto\n"
       << "D: Loop or condition     E: Class/template decl    F: Templates/types\n"
       << "G: Input/output          H: Exception handling     I: Threads/mutexes\n"
       << "J: STL structures        Q: Quit                   R: Show mistakes\n";
  cout << "\033[0m";
}

void print_question(const Entry& q, int question_num) {
  cout << "\n\033[90m[Question " << question_num << "] What does this line do?\033[0m\n\n";
  cout << "\033[93m" << q.code << "\033[0m\n\n";
}

void print_stats(size_t total, size_t correct, size_t incorrect) {
  cout << "\n------------------------------\n";
  cout << "Total: " << total
       << "  ✅: " << correct
       << "  ❌: " << incorrect << "\n";
  cout << "------------------------------\n";
}

void wait_for_any_key() {
  cout << "\nPress any key to continue...";
  get_single_char();
}

void print_report(const map<string, pair<char, int>>& mistakes) {
  clear_screen();
  cout << "\033[91m=== Mistake Report ===\033[0m\n\n";
  if (mistakes.empty()) {
    cout << "No mistakes yet!\n";
  } else {
    for (const auto& [line, data] : mistakes) {
      char correct = data.first;
      int count = data.second;
      cout << "- " << line << "\n";
      cout << "    → Correct answer: " << correct << " (" << explain(correct) << ")";
      if (count > 1) cout << "  (" << count << "x)";
      cout << "\n\n";
    }
  }
}

int main() {
  vector<Entry> quiz = load_quiz("quiz.txt");
  random_device rd;
  mt19937 g(rd());
  shuffle(quiz.begin(), quiz.end(), g);

  size_t asked = 0, correct = 0, incorrect = 0;
  size_t question_num = 1;
  map<string, pair<char, int>> mistakes;

  for (const auto& q : quiz) {
    clear_screen();
    print_question(q, question_num);
    print_key_legend();
    cout << "Your answer (A–J, R or Q): ";

    char input = 0;
    do {
      input = toupper(get_single_char());
    } while( explain(input)=="" && input != 'Q' && input != 'R' );

    cout << input << "\n";
    if (input == 'Q') {
      cout << "Are you sure you want to quit? (y/n): ";
      char confirm = tolower(get_single_char());
      cout << confirm << "\n";
      if (confirm == 'y') break;
      else continue;
    }

    if (input == 'R') {
      print_report(mistakes);
      wait_for_any_key();
      continue;
    }

    ++asked;
    if (input == q.answer) {
      ++correct;
      cout << "✅ Correct!\n";
    } else {
      ++incorrect;
      cout << "❌ Incorrect. Correct answer: " << q.answer << " (" << explain(q.answer) << ")\n";
      mistakes[q.code].first = q.answer;
      mistakes[q.code].second++;
      print_stats(asked, correct, incorrect);
      //usleep(1000000);
      wait_for_any_key();
    }

    ++question_num;
  }

  clear_screen();
  cout << "=== Final Stats ===\n";
  print_stats(asked, correct, incorrect);
  print_report(mistakes);
  print_secret_ascii();
  cout << "Thanks for playing!\n";
  return 0;
}


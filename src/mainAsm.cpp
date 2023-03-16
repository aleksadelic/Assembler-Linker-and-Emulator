#include <iostream>
#include <vector>
#include <utility>
#include "../inc/assembler.h"

using namespace std;
  
enum Type {DIRECTIVE, SYMBOL, COMMAND};

vector<pair<string, Type>> m;

string enumStrings[] = {"DIRECTIVE", "SYMBOL", "COMMAND"};

Assembler* assembler = 0;

int main(int argc, char* argv[]) {

  if (argc < 4) {
    cout << "Neispravni argumenti!" << endl;
  }
  string option = argv[1];
  if (option != "-o") {
    cout << "Neispravna opcija" << endl;
  }
  string output_file = argv[2];
  string input_file = argv[3];

  Assembler myAssembler(input_file, output_file);
  assembler = &myAssembler;
  assembler->compile();
}

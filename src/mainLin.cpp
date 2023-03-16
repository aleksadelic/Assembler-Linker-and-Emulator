#include <iostream>
#include "../inc/linker.h"

using namespace std;

int main(int argc, char* argv[]) {

  if (argc < 5) {
    cout << "Nedovoljno argumenata!" << endl;
    return 1;
  }

  vector<string> input_files;
  string output_file;
  bool hex = false;

  for (int i = 1; i < argc; i++) {
    string arg = argv[i];
    if (arg == "-hex") {
      hex = true;
    } else if (arg == "-o") {
      if (i + 1 == argc) {
        cout << "Neispravni argumenti!" << endl;
        return 1;
      }
      output_file = argv[i + 1]; 
      i++;
    } else {
      input_files.push_back(arg);
    }
  }

  if (!hex) {
    cout << "Neispravni argumenti!" << endl;
    return 1;
  }
  Linker linker(input_files, output_file);
  linker.process_data();
}

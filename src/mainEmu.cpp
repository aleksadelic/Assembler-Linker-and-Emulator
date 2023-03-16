#include <iostream>
#include "../inc/emulator.h"

using namespace std;

int main(int argc, char* argv[]) {

  if (argc < 2) {
    cout << "Nedovoljno argumenata!" << endl;
    return 1;
  }
  string input_file_path = argv[1];
  Emulator emulator(input_file_path);
  emulator.emulate();
}
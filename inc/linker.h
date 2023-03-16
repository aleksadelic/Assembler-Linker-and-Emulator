#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <utility>
#include "tableEntryLin.h"

using namespace std;

class Linker {
private:
  vector<Symbol> symbolTable;
  vector<Section> sectionTable;
  vector<vector<Section>*> sectionMap;
  map<pair<int, int>, int> sectionIdMap;
  map<pair<int, int>, int> symbolIdMap;
  map<int, int> sectionOffsetMap;
  vector<Relocation> relocationTable;
  vector<char> data;

  string output_file_path;
  ofstream output_file;
  ofstream text_file;

public:
  Linker(vector<string> input_files_paths, string output_file_path);

  void read_file(string file_name, int file_id);  
  void update_symbol_table();
  void update_relocation_table();
  void combine_sections();
  void resolve_relocations();
  void generate_binary_output_file();
  void generate_text_output_file();
  void process_data();

  void print_symbol_table();
  void print_relocation_table();
};
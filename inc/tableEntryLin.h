#include <iostream>
#include <vector>
using namespace std;

enum relocationType {ABS, PCREL};

struct Symbol {
  string label;
  int section_id;
  int offset;
  bool is_local;
  bool is_defined;
  int id;
  int size;
  static int next_id;
  int file_id;

  Symbol(string label, int section, int offset, bool is_local, bool is_defined, int id, int size, int file_id) : 
    label(label), section_id(section), offset(offset), is_local(is_local), is_defined(is_defined), id(id), size(size), file_id(file_id) {}

};

struct Relocation {
  int section_id;
  int offset;
  relocationType type;
  string symbol;
  int symbol_id;
  int addend;
  int file_id;

  Relocation(int section, int offset, relocationType type, string symbol, int id, int addend, int file_id) : 
    section_id(section), offset(offset), type(type), symbol(symbol), symbol_id(id), addend(addend), file_id(file_id) {}
};

struct Section {
  string section;
  int offset;
  int section_id;
  int symbol_id;
  int size;
  vector<char> data;
  static int next_id;
  static int next_id_combined;

  Section(string section, int offset, int section_id, int symbol_id, int size) : section(section), offset(offset),
    section_id(section_id), symbol_id(symbol_id), size(size) {}

  void addData(char c);
};
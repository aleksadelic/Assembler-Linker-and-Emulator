#include <iostream>
#include <vector>
using namespace std;

enum relocationType {ABS, PCREL};

struct ForwardReference;

struct Symbol {
  string label;
  int section_id;
  int offset;
  bool is_local;
  bool is_defined;
  int id;
  int size;
  static int next_id;
  vector<ForwardReference> references; // to patch

  Symbol(string label, int section, int offset, bool is_local, bool is_defined, int id, int size) : 
    label(label), section_id(section), offset(offset), is_local(is_local), is_defined(is_defined), id(id), size(size) {}

};

struct Relocation {
  int section_id;
  int offset;
  relocationType type;
  string symbol;
  int symbol_id;
  int addend;

  Relocation(int section, int offset, relocationType type, string symbol, int id, int addend) : 
    section_id(section), offset(offset), type(type), symbol(symbol), symbol_id(id), addend(addend) {}
};

struct Section {
  string section;
  int offset;
  int section_id;
  int symbol_id;
  int size;
  vector<char> data;
  static int next_id;

  Section(string section, int offset, int section_id, int symbol_id, int size) : section(section), offset(offset),
    section_id(section_id), symbol_id(symbol_id), size(size) {}

  void addData(char c);
};

struct ForwardReference {
  int sectionId;
  int offset;
  relocationType type;

  ForwardReference(int sectionId, int offset, relocationType rel_type): sectionId(sectionId), offset(offset), type(rel_type) {}
};
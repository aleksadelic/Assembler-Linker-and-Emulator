#include <iostream>
#include <fstream>
#include <vector>
#include "tableEntryAsm.h"

using namespace std;

class Assembler {
private:
  string input_file_path;
  string output_file_path;
  ofstream output_file;
  ofstream text_file;
  bool stop = false;

  int locationCounter = 0;
  string currentSection;
  int currentSectionId = 0;

  vector<Symbol> symbolTable;
  vector<Section> sectionTable;
  vector<Relocation> relocationTable;

public:

  enum AddressType { IMMED, REGDIR, REGIND, REGIND_DIS, MEMDIR, REGDIR_DIS, REGDIR_PCREL, REGIND_PCREL };

  Assembler(string input_file_path, string output_file_path);
  ~Assembler();

  int compile();

  int process_directive(string directive);
  int process_label(string label);
  int process_instruction_with_no_arguments(string instruction);
  int process_register_instruction_with_one_argument(string instruction);
  int process_register_instruction_with_two_arguments(string instruction);
  int process_one_operand_instruction(string instruction, AddressType type);
  int process_register_operand_instruction(string instruction, AddressType type);

  static vector<string> split_string(string str);
  static int determine_register_number(string reg);
  static bool check_if_literal(string str);
  static bool check_if_hex(string str);
  static int hex_to_dec(string str);
  void increment_location_counter();
  Symbol find_symbol(string label);
  int process_operand(string opr, int offset, AddressType type);
  int check_if_label_exists(string label);
  void backpatch(int symbol_id);
  void make_relocation_records();

  void print_symbol_table();
  void print_relocation_table();
  void generate_binary_output_file();
  void generate_text_output_file();
};
#include "../inc/assembler.h"
#include <sstream>
#include <algorithm>
#include <iomanip>

extern int yylex();
extern FILE* yyin;

Assembler::Assembler(string ipath, string opath) : input_file_path(ipath), output_file_path(opath) {
  output_file.open(output_file_path, ios::binary);
  output_file_path.pop_back();
  string output_txt_path = output_file_path + "txt";
  text_file.open(output_txt_path);
  Section initialSection("initial", 0, 0, 0, 0);
  sectionTable.push_back(initialSection);
  symbolTable.push_back(Symbol("initial", 0, 0, true, true, 0, 0));
}

Assembler::~Assembler() {}

int Assembler::compile() {

  // open a file handle to a particular file:
  FILE *myfile = fopen(input_file_path.c_str(), "r");
  // make sure it's valid:
  if (!myfile) {
    cout << "I can't open myfile!" << endl;
    return -1;
  }
  // set lex to read from it instead of defaulting to STDIN:
  yyin = myfile;
  
  // lex through the input:
  while(!stop && yylex());

  make_relocation_records();
  //print_symbol_table();
  //print_relocation_table();  

  generate_binary_output_file();
  generate_text_output_file();

  return 0;
}

int Assembler::process_directive(string directive) {
  vector<string> parsed_directive = split_string(directive);
  string dir = parsed_directive[0];

  if (dir == ".global") {
    for(int i = 1; i < parsed_directive.size(); i++) {
      symbolTable.push_back(Symbol(parsed_directive[i], currentSectionId, locationCounter, false, true, Symbol::next_id++, 0));
    }

  } else if (dir == ".extern") {
    for(int i = 1; i < parsed_directive.size(); i++) {
      symbolTable.push_back(Symbol(parsed_directive[i], 0, 0, false, false, Symbol::next_id++, 0));
    }

  } else if (dir == ".section") {
    string section = parsed_directive[1];
    if (currentSectionId != 0)
      symbolTable[currentSectionId].size = locationCounter;
    symbolTable.push_back(Symbol(section, Section::next_id, locationCounter, true, true, Symbol::next_id, 0));
    sectionTable.push_back(Section(section, locationCounter, Section::next_id, Symbol::next_id++, 0));
    currentSection = section;
    currentSectionId = Section::next_id++;
    locationCounter = 0;
  } else if (dir == ".word") {
    for (int i = 1; i < parsed_directive.size(); i++) {
      int num = process_operand(parsed_directive[i], 0, IMMED);
      sectionTable[currentSectionId].addData(num & 0xFF);
      sectionTable[currentSectionId].addData(num >> 8 & 0xFF);
      locationCounter += 2;
    }
  } else if (dir == ".skip") {
      int num = atoi(parsed_directive[1].c_str());
      for(int i = 0; i < num; i++) {
        sectionTable[currentSectionId].addData(0x00);
      }
      locationCounter += num;
  } else if (dir == ".end") {
      stop = true;
  } else return 1;

  return 0;
}

int Assembler::process_label(string label) {
  label.erase(remove(label.begin(), label.end(), ':'), label.end());
  int id = check_if_label_exists(label);
  if (id != 0) {
    symbolTable[id].is_defined = true;
    symbolTable[id].is_local = true;
    symbolTable[id].offset = locationCounter;
    symbolTable[id].section_id = currentSectionId;
    backpatch(id);
  } else {
    symbolTable.push_back(Symbol(label, currentSectionId, locationCounter, true, true, Symbol::next_id++, 0));
  }
  return 0;
}

int Assembler::process_instruction_with_no_arguments(string instruction) {
  if (instruction == "halt") {
    locationCounter += 1;
    sectionTable[currentSectionId].addData(0x00);
  } else if (instruction == "iret") {
    locationCounter += 1;
    sectionTable[currentSectionId].addData(0x20);
  } else if (instruction == "ret") {
    locationCounter += 1;
    sectionTable[currentSectionId].addData(0x40);
  } else return 1;
  return 0;
}

int Assembler::process_register_instruction_with_one_argument(string instruction) {
  vector<string> parsed_instruction = split_string(instruction);

  string ins = parsed_instruction[0];
  string reg = parsed_instruction[1];

  int reg_num = determine_register_number(reg);
  int sp_num = 6;

  if (ins == "int") {
    locationCounter += 2;
    //ins_code = 0x10 << 8 | reg_num << 4 | 0xF;
    sectionTable[currentSectionId].addData(0x10);
    sectionTable[currentSectionId].addData(reg_num << 4 | 0xF);
  } else if (ins == "push") {
    locationCounter += 3;
    //ins_code = 0xB0 << 16 | (reg_num << 4 | sp_num) << 8 | (0x1 << 4 | 0x1);
    sectionTable[currentSectionId].addData(0xB0);
    sectionTable[currentSectionId].addData(reg_num << 4 | sp_num);
    sectionTable[currentSectionId].addData(0x1 << 4 | 0x2);
  } else if (ins == "pop") {
    locationCounter += 3;
    //ins_code = 0xA0 << 16 | (reg_num << 4 | sp_num) << 8 | (0x4 << 4 | 0x1);
    sectionTable[currentSectionId].addData(0xA0);
    sectionTable[currentSectionId].addData(reg_num << 4 | sp_num);
    sectionTable[currentSectionId].addData(0x4 << 4 | 0x2);
  } else if (ins == "not") {
    locationCounter += 2;
    reg_num <<= 4;
    sectionTable[currentSectionId].addData(0x80);
    sectionTable[currentSectionId].addData(reg_num);
  } else return 1;

  return 0;
}

int Assembler::process_register_instruction_with_two_arguments(string instruction) {
  vector<string> parsed_instruction = split_string(instruction);

  string ins = parsed_instruction[0];
  string reg1 = parsed_instruction[1];
  string reg2 = parsed_instruction[2];

  int reg1_num = determine_register_number(reg1);
  int reg2_num = determine_register_number(reg2);

  int reg_byte = reg1_num << 4 | reg2_num;

  if (ins == "xchg") {
    sectionTable[currentSectionId].addData(0x60);
  } else if (ins == "add") {
    sectionTable[currentSectionId].addData(0x70);
  } else if (ins == "sub") {
    sectionTable[currentSectionId].addData(0x71);
  } else if (ins == "mul") {
    sectionTable[currentSectionId].addData(0x72);
  } else if (ins == "div") {
    sectionTable[currentSectionId].addData(0x73);
  } else if (ins == "cmp") {
    sectionTable[currentSectionId].addData(0x74);
  } else if (ins == "and") {
    sectionTable[currentSectionId].addData(0x81);
  } else if (ins == "or") {
    sectionTable[currentSectionId].addData(0x82);
  } else if (ins == "xor") {
    sectionTable[currentSectionId].addData(0x83);
  } else if (ins == "test") {
    sectionTable[currentSectionId].addData(0x84);
  } else if (ins == "shl") {
    sectionTable[currentSectionId].addData(0x90);
  } else if (ins == "shr") {
    sectionTable[currentSectionId].addData(0x91);
  } else return 1;

  sectionTable[currentSectionId].addData(reg_byte);
  locationCounter += 2;
  return 0;
}

int Assembler::process_one_operand_instruction(string instruction, AddressType type) {
  vector<string> parsed_instruction = split_string(instruction);
  string ins = parsed_instruction[0];
  string opr = parsed_instruction[1];

  int addr_mode = type;
  int opr_num;
  int reg_num;
  vector<char> ins_code;
  string opr2;
  switch (type) {
    case IMMED:
    // ins_code = 0xF0 << 24 | addr_mode << 16 | opr_num;
    opr_num = process_operand(opr, 3, type);
    ins_code.push_back(0xF0);
    ins_code.push_back(addr_mode);
    ins_code.push_back(opr_num & 0xFF);
    ins_code.push_back(opr_num >> 8 & 0xFF);
    locationCounter += 5;
    break;
    
    case REGDIR:
    reg_num = determine_register_number(opr);
    // ins_code = (0xF << 4 | reg_num) << 8 | addr_mode;
    ins_code.push_back(0xF << 4 | reg_num);
    ins_code.push_back(addr_mode);
    locationCounter += 3;
    break;

    case REGDIR_DIS:
    reg_num = determine_register_number(opr);
    opr2 = parsed_instruction[2];
    opr_num = process_operand(opr2, 3, type);
    // ins_code = (0xF << 4 | reg_num) << 24 | addr_mode << 16 | opr_num;
    ins_code.push_back(0xF << 4 | reg_num);
    ins_code.push_back(addr_mode);
    ins_code.push_back(opr_num & 0xFF);
    ins_code.push_back(opr_num >> 8 & 0xFF);
    locationCounter += 5;
    break;

    case REGIND: 
    reg_num = determine_register_number(opr);
    // ins_code = (0xF << 4 | reg_num) << 8 | addr_mode;
    ins_code.push_back(0xF << 4 | reg_num);
    ins_code.push_back(addr_mode);
    locationCounter += 3;
    break;

    case REGIND_DIS:
    reg_num = determine_register_number(opr);
    opr2 = parsed_instruction[2];
    opr_num = process_operand(opr2, 3, type);
    // ins_code = (0xF << 4 | reg_num) << 24 | addr_mode << 16 | opr_num;
    ins_code.push_back(0xF << 4 | reg_num);
    ins_code.push_back(addr_mode);
    ins_code.push_back(opr_num & 0xFF);
    ins_code.push_back(opr_num >> 8 & 0xFF);
    locationCounter += 5;
    break;

    case MEMDIR:
    opr_num = process_operand(opr, 3, type);
    // ins_code = 0xF0 << 24 | addr_mode << 16 | opr_num;
    ins_code.push_back(0xF0);
    ins_code.push_back(addr_mode);
    ins_code.push_back(opr_num & 0xFF);
    ins_code.push_back(opr_num >> 8 & 0xFF);
    locationCounter += 5;
    break;

    case REGDIR_PCREL:
    opr_num = process_operand(opr, 3, type);
    opr_num -= locationCounter + 5;
    reg_num = 7; // PC
    // ins_code = (0xF << 4 | reg_num) << 24 | addr_mode << 16 | opr_num;
    ins_code.push_back(0xF << 4 | reg_num);
    ins_code.push_back(REGDIR_DIS);
    ins_code.push_back(opr_num & 0xFF);
    ins_code.push_back(opr_num >> 8 & 0xFF);
    locationCounter += 5;
    break;

    case REGIND_PCREL: // Ne postoji u ovom slucaju?
    opr_num = process_operand(opr, 3, type);
    opr_num -= locationCounter + 5;
    reg_num = 7; // PC
    // ins_code = (0xF << 4 | reg_num) << 24 | addr_mode << 16 | opr_num;
    ins_code.push_back(0xF << 4 | reg_num);
    ins_code.push_back(REGIND_DIS);
    ins_code.push_back(opr_num & 0xFF);
    ins_code.push_back(opr_num >> 8 & 0xFF);
    locationCounter += 5;
    break;
  }

  if (ins == "call") {
    sectionTable[currentSectionId].addData(0x30);
  } else if (ins == "jmp") {
    sectionTable[currentSectionId].addData(0x50);
  } else if (ins == "jeq") {
    sectionTable[currentSectionId].addData(0x51);
  } else if (ins == "jne") {
    sectionTable[currentSectionId].addData(0x52);
  } else if (ins == "jgt") {
    sectionTable[currentSectionId].addData(0x53);
  } else return 1;

  for (char c: ins_code) {
    sectionTable[currentSectionId].addData(c);
  }
  
  return 0;
}

int Assembler::process_register_operand_instruction(string instruction, AddressType type) {
  vector<string> parsed_instruction = split_string(instruction);

  string ins = parsed_instruction[0];
  string reg = parsed_instruction[1];
  string opr = parsed_instruction[2];

  int addr_mode = type;
  int opr_num;
  int reg_num = determine_register_number(reg);
  int reg2_num;
  vector<char> ins_code;
  string opr2;

  switch (type) {
    case IMMED:
    opr_num = process_operand(opr, 3, type);
    // ins_code = reg_num << 28 | addr_mode << 16 | opr_num;
    ins_code.push_back(reg_num << 4);
    ins_code.push_back(addr_mode);
    ins_code.push_back(opr_num & 0xFF);
    ins_code.push_back(opr_num >> 8 & 0xFF);
    locationCounter += 5;
    break;
    
    case REGDIR:
    reg2_num = determine_register_number(opr);
    // ins_code = (reg_num << 4 | reg2_num) << 8 | addr_mode;
    ins_code.push_back(reg_num << 4 | reg2_num);
    ins_code.push_back(addr_mode);
    locationCounter += 3;
    break;

    case REGDIR_DIS:
    reg2_num = determine_register_number(opr);
    opr2 = parsed_instruction[3];
    opr_num = process_operand(opr2, 3, type);
    // ins_code = (reg_num << 4 | reg2_num) << 24 | addr_mode << 16 | opr_num;
    ins_code.push_back(reg_num << 4 | reg2_num);
    ins_code.push_back(addr_mode);
    ins_code.push_back(opr_num & 0xFF);
    ins_code.push_back(opr_num >> 8 & 0xFF);
    locationCounter += 5;
    break;

    case REGIND: 
    reg2_num = determine_register_number(opr);
    // ins_code = (reg_num << 4 | reg2_num) << 8 | addr_mode;
    ins_code.push_back(reg_num << 4 | reg2_num);
    ins_code.push_back(addr_mode);
    locationCounter += 3;
    break;

    case REGIND_DIS:
    reg2_num = determine_register_number(opr);
    opr2 = parsed_instruction[3];
    opr_num = process_operand(opr2, 3, type);
    // ins_code = (reg_num << 4 | reg2_num) << 24 | addr_mode << 16 | opr_num;
    ins_code.push_back(reg_num << 4 | reg2_num);
    ins_code.push_back(addr_mode);
    ins_code.push_back(opr_num & 0xFF);
    ins_code.push_back(opr_num >> 8 & 0xFF);
    locationCounter += 5;
    break;

    case MEMDIR:
    opr_num = process_operand(opr, 3, type);
    // ins_code = reg_num << 24 | addr_mode << 16 | opr_num;
    ins_code.push_back(reg_num << 4);
    ins_code.push_back(addr_mode);
    ins_code.push_back(opr_num & 0xFF);
    ins_code.push_back(opr_num >> 8 & 0xFF);
    locationCounter += 5;
    break;

    case REGDIR_PCREL:
    opr2 = parsed_instruction[2];
    opr_num = process_operand(opr2, 3, type);
    opr_num -= locationCounter + 5;
    reg2_num = 7; // PC
    // ins_code = (reg_num << 4 | reg2_num) << 24 | addr_mode << 16 | opr_num;
    ins_code.push_back(reg_num << 4 | reg2_num);
    ins_code.push_back(REGDIR_DIS);
    ins_code.push_back(opr_num & 0xFF);
    ins_code.push_back(opr_num >> 8 & 0xFF);
    locationCounter += 5;
    break;

    case REGIND_PCREL:
    opr2 = parsed_instruction[2];
    opr_num = process_operand(opr2, 3, type);
    opr_num -= locationCounter + 5;
    reg2_num = 7; // PC
    // ins_code = (reg_num << 4 | reg2_num) << 24 | addr_mode << 16 | opr_num;
    ins_code.push_back(reg_num << 4 | reg2_num);
    ins_code.push_back(REGIND_DIS);
    ins_code.push_back(opr_num & 0xFF);
    ins_code.push_back(opr_num >> 8 & 0xFF);
    locationCounter += 5;
    break;
  }

  if (ins == "ldr") {
    sectionTable[currentSectionId].addData(0xA0);
  } else if (ins == "str") {
    sectionTable[currentSectionId].addData(0xB0);
  } else return 1;

  for(char c: ins_code) {
    sectionTable[currentSectionId].addData(c);
  }

  return 0;
}

vector<string> Assembler::split_string(string str) {
  vector<string> ret;
  str.erase(remove(str.begin(), str.end(), ','), str.end());
  str.erase(remove(str.begin(), str.end(), '$'), str.end());
  str.erase(remove(str.begin(), str.end(), '%'), str.end());
  str.erase(remove(str.begin(), str.end(), '['), str.end());
  str.erase(remove(str.begin(), str.end(), ']'), str.end());
  str.erase(remove(str.begin(), str.end(), '+'), str.end());
  str.erase(remove(str.begin(), str.end(), '*'), str.end());
  stringstream ss(str);
  string word;
  while (ss >> word) {
      ret.push_back(word);
  }
  return ret;
}

int Assembler::determine_register_number(string reg) {
  if (reg == "psw")
    return 8;
  return reg[1] - 48;
}

bool Assembler::check_if_literal(string str) {
  for (int i = 0; i < str.size(); i++) {
    if (!(str[i] >= '0' && str[i] <= '9') && !(str[i] >= 'A' && str[i] <= 'F')) {
      if (str[i] == 'x' && i == 1)
        continue;
      else 
        return false;
    }
  }

  return true;
}

bool Assembler::check_if_hex(string str) {
  if (str[1] == 'x')
    return true;
  else
    return false;
}

int Assembler::hex_to_dec(string str) {
  int num;
  stringstream ss;
  ss << std::hex << str;
  ss >> num;
  return num;
}

void Assembler::increment_location_counter() {
  locationCounter++;
}

Symbol Assembler::find_symbol(string label) {
  for (Symbol symbol: symbolTable) {
    if (symbol.label == label) 
      return symbol;
  }
  return symbolTable[0]; // entry 0 - sign that symbol is not found
}

int Assembler::process_operand(string opr, int offset, AddressType type) {
  int opr_num = 0;
  if (check_if_literal(opr)) {
    if (check_if_hex(opr)) {
      opr_num = hex_to_dec(opr);
    } else {
      opr_num = atoi(opr.c_str());
    }
  } else {
    Symbol symbol = find_symbol(opr);
    relocationType rel_type = type == REGDIR_PCREL || type == REGIND_PCREL ? PCREL : ABS;
    if (symbol.id != 0) {
      if(symbol.is_defined) {
        opr_num = symbol.offset;
        if (symbol.section_id != currentSectionId) {
          relocationTable.push_back(Relocation(sectionTable[currentSectionId].section_id, locationCounter+offset, 
            rel_type, symbol.label, symbol.id, -2));
        }
      }
      else {
        symbolTable[symbol.id].references.push_back(ForwardReference(currentSectionId, locationCounter+offset, rel_type));
      }
    } else {
      Symbol undefinedSymbol(opr, 0, 0, false, false, Symbol::next_id++, 0);
      undefinedSymbol.references.push_back(ForwardReference(currentSectionId, locationCounter+offset, rel_type));
      symbolTable.push_back(undefinedSymbol);
    }
  }
  return opr_num;
}

int Assembler::check_if_label_exists(string label) {
  for (Symbol symbol: symbolTable) {
    if (symbol.label == label)
      return symbol.id;
  }
  return 0;
}

void Assembler::backpatch(int id) {
  Symbol symbol = symbolTable[id];
  for(ForwardReference ref: symbol.references) {
    sectionTable[ref.sectionId].data[ref.offset] = symbol.offset;
    if (symbol.section_id != sectionTable[ref.sectionId].section_id) {
      /////////////////// RELOKACIONI ZAPIS
      relocationTable.push_back(Relocation(sectionTable[ref.sectionId].section_id, ref.offset, ref.type, symbol.label, symbol.id, -2));
    }
  }
  symbolTable[id].references.clear();
}

void Assembler::make_relocation_records() {
  for (Symbol symbol: symbolTable) {
    for (ForwardReference ref: symbol.references) {
      relocationTable.push_back(Relocation(sectionTable[ref.sectionId].section_id, ref.offset, ref.type, symbol.label, symbol.id, -2));
    }
  }
}

void Assembler::print_symbol_table() {
  cout << "TABELA SIMBOLA:" << endl;
  cout << "====================================================" << endl;
  for (int i = 0; i < symbolTable.size(); i++) {
    Symbol curr = symbolTable[i];
    cout << curr.section_id << " " << sectionTable[curr.section_id].section << " " << curr.label << " " << curr.offset << " " << curr.id << endl;
  }
  cout << "====================================================" << endl;
}

void Assembler::print_relocation_table() {
  cout << "TABELA RELOKACIJA:" << endl;
  cout << "====================================================" << endl;
  for (int i = 0; i < relocationTable.size(); i++) {
    Relocation curr = relocationTable[i];
    cout << curr.section_id << " " << sectionTable[curr.section_id].section << " " << curr.offset << " " << curr.symbol << endl;
  }
  cout << "====================================================" << endl;
}

void Assembler::generate_binary_output_file() {
  // write symbol table
  int number_of_symbols = symbolTable.size();
  output_file.write((char*)&number_of_symbols, sizeof(number_of_symbols));
  for (Symbol entry: symbolTable) {
    int string_length = entry.label.size();
    const char* label = entry.label.c_str();
    output_file.write((char*)&string_length, sizeof(string_length));
    output_file.write((char*)label, string_length);
    /*string_length = entry.section.size();
    con/st char* section = entry.section.c_str();
    output_file.write((char*)&string_length, sizeof(string_length));
    output_file.write((char*)section, string_length);*/
    output_file.write((char*)&entry.section_id, sizeof(entry.section_id));
    output_file.write((char*)&entry.id, sizeof(entry.id));
    output_file.write((char*)&entry.offset, sizeof(entry.offset));
    output_file.write((char*)&entry.is_local, sizeof(entry.is_local));
    output_file.write((char*)&entry.is_defined, sizeof(entry.is_defined));
    output_file.write((char*)&entry.size, sizeof(entry.size));
  }

  // write relocation table
  int number_of_relocations = relocationTable.size();
  output_file.write((char*)&number_of_relocations, sizeof(number_of_relocations));
  for (Relocation entry: relocationTable) {
    /*int string_length = entry.section.size();
    const char* section = entry.section.c_str();
    output_file.write((char*)&string_length, sizeof(string_length));
    output_file.write((char*)section, string_length);*/
    output_file.write((char*)&entry.section_id, sizeof(entry.section_id));
    int string_length = entry.symbol.size();
    const char* label = entry.symbol.c_str();
    output_file.write((char*)&string_length, sizeof(string_length));
    output_file.write((char*)label, string_length);
    output_file.write((char*)&entry.symbol_id, sizeof(entry.symbol_id));
    output_file.write((char*)&entry.offset, sizeof(entry.offset));
    output_file.write((char*)&entry.type, sizeof(entry.type));
    output_file.write((char*)&entry.addend, sizeof(entry.addend));
  }

  int sections_size = 0;
  for (Section entry: sectionTable) {
    sections_size += entry.data.size();
  }
  output_file.write((char*)&sections_size, sizeof(sections_size));

  // write section data
  int counter = 0;
  vector<int> offsets;
  for (Section entry: sectionTable) {
    offsets.push_back(counter);
    counter += entry.data.size();
    char arr[entry.data.size()];
    copy(entry.data.begin(), entry.data.end(), arr);
    output_file.write(arr, entry.data.size());
  }

  // write section header
  int number_of_sections = sectionTable.size();
  output_file.write((char*)&number_of_sections, sizeof(number_of_sections));
  for (int i = 0; i < sectionTable.size(); i++) {
    Section entry = sectionTable[i];
    int string_length = entry.section.size();
    const char* section = entry.section.c_str();
    output_file.write((char*)&string_length, sizeof(string_length));
    output_file.write((char*)section, string_length);
    output_file.write((char*)&entry.section_id, sizeof(entry.section_id));
    output_file.write((char*)&entry.symbol_id, sizeof(entry.symbol_id));
    output_file.write((char*)&entry.offset, sizeof(entry.offset));
    entry.size = entry.data.size();
    output_file.write((char*)&entry.size, sizeof(entry.size));
    int file_offset = offsets[i];
    output_file.write((char*)&file_offset, sizeof(file_offset));
  }

  output_file.close();
}

void Assembler::generate_text_output_file() {
  text_file << "#.symtab" << endl;

  string local_string[2] = {"GLOBAL", "LOCAL"};

  text_file << "id\tlabel\tsection\toffset\tlocal\tsize" << endl;
  for (Symbol entry: symbolTable) {
    text_file << entry.id << '\t' << entry.label << '\t' << entry.section_id << '\t' <<
      entry.offset << '\t' << local_string[entry.is_local] << '\t' << entry.size << endl;
  }
  text_file << endl;
  string type_string[2] = {"ABS", "PCREL"};
  text_file << "#.reltab" << endl;
  text_file << "section\tsymbol\tsymbol id\toffset\trelocation type\taddend" << endl;
  for (Relocation entry: relocationTable) {
    text_file << entry.section_id << '\t' << entry.symbol << '\t' << entry.symbol_id << '\t' <<
      entry.offset << '\t' << type_string[entry.type] << '\t' << entry.addend << endl;
  }

  for (Section entry: sectionTable) {
    if (entry.section_id == 0) continue;
    text_file << "#." << entry.section;
    for (int i = 0; i < entry.data.size(); i++) {
      if (i % 8 == 0) text_file << endl;
      else if (i % 4 == 0) text_file << '\t';
      text_file << hex << setfill('0') << setw(2) << ((int)entry.data[i] & 0xFF) << ' ';
    }
    text_file << endl << endl;
  }

  text_file.close();
}
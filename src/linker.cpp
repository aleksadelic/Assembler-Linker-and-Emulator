#include "../inc/linker.h"
#include <fstream>
#include <ios>
#include <utility>
#include <iterator>
#include <iomanip>

Linker::Linker(vector<string> input_files_paths, string output_file): output_file_path(output_file) {
  Section initialSection("initial", 0, 0, 0, 0);
  sectionTable.push_back(initialSection);
  symbolTable.push_back(Symbol("initial", 0, 0, true, false, 0, 0, -1));
  for (int i = 0; i < input_files_paths.size(); i++) {
    read_file(input_files_paths[i], i);
  }
}

void Linker::read_file(string file_name, int file_id) {
  ifstream file(file_name, ios::binary);

  // creating symbol table from input file data
  int number_of_symbols;
  file.read((char*)&number_of_symbols, sizeof(number_of_symbols));
  for (int i = 0; i < number_of_symbols; i++) {
    int string_length;
    file.read((char*)&string_length, sizeof(string_length));
    string label("");
    for (int j = 0; j < string_length; j++) {
      label.push_back((char)file.get());
    }
    int section_id;
    file.read((char*)&section_id, sizeof(section_id));
    int id;
    file.read((char*)&id, sizeof(id));
    int offset;
    file.read((char*)&offset, sizeof(offset));
    bool is_local = (bool)file.get();
    bool is_defined = (bool)file.get();
    int size;
    file.read((char*)&size, sizeof(size));

    int old_id = id;
    id = Symbol::next_id++;
    symbolIdMap.insert(pair<pair<int, int>, int>(pair<int, int>(file_id, old_id), id));
    Symbol symbol(label, section_id, offset, is_local, is_defined, id, size, file_id);
    symbolTable.push_back(symbol);
  }

  // creating relocation table from file data
  int number_of_relocations;
  file.read((char*)&number_of_relocations, sizeof(number_of_relocations));
  for (int i = 0; i < number_of_relocations; i++) {
    int section_id;
    file.read((char*)&section_id, sizeof(section_id));
    int string_length;
    file.read((char*)&string_length, sizeof(string_length));
    string label("");
    for (int j = 0; j < string_length; j++) {
      label.push_back((char)file.get());
    }
    int symbol_id;
    file.read((char*)&symbol_id, sizeof(symbol_id));
    int offset;
    file.read((char*)&offset, sizeof(offset));
    relocationType type;
    file.read((char*)&type, sizeof(type));
    int addend;
    file.read((char*)&addend, sizeof(addend));

    Relocation reloc(section_id, offset, type, label, symbol_id, addend, file_id);
    relocationTable.push_back(reloc);
  }

  // reading section data from file
  int sections_size;
  file.read((char*)&sections_size, sizeof(sections_size));
  char data[sections_size];
  file.read(data, sizeof(data));

  // reading section header from file
  vector<int> data_offsets;
  int current = 0;
  int number_of_sections;
  file.read((char*)&number_of_sections, sizeof(number_of_sections));
  for (int i = 0; i < number_of_sections; i++) {
    int string_length;
    file.read((char*)&string_length, sizeof(string_length));
    string section("");
    for (int j = 0; j < string_length; j++) {
      section.push_back((char)file.get());
    }
    int section_id;
    file.read((char*)&section_id, sizeof(section_id));
    int symbol_id;
    file.read((char*)&symbol_id, sizeof(symbol_id));
    int offset;
    file.read((char*)&offset, sizeof(offset));
    int size;
    file.read((char*)&size, sizeof(size));
    int data_offset;
    file.read((char*)&data_offset, sizeof(data_offset));
    data_offsets.push_back(data_offset);
    vector<char> section_data(data + current, data + current + size);
    current += size;

    int old_id = section_id;
    section_id = Section::next_id++;
    sectionIdMap.insert(pair<pair<int, int>, int>(pair<int, int>(file_id, old_id), section_id));
    Section newSection(section, offset, section_id, symbol_id, size);
    newSection.data = section_data;

    /*auto search = sectionMap.find(section);
    if (search != sectionMap.end()) {
      search->second->push_back(newSection);
    } else {
      vector<Section>* vec = new vector<Section>();
      vec->push_back(newSection);
      sectionMap.insert(pair<string, vector<Section>*>(section, vec));
    }*/
    bool found = false;
    for (vector<Section>* sectionVector: sectionMap) {
      if (sectionVector->front().section == section) {
        sectionVector->push_back(newSection);
        found = true;
        break;
      }
    }
    if (!found) {
      vector<Section>* newVector = new vector<Section>();
      newVector->push_back(newSection);
      sectionMap.push_back(newVector);
    }
  }
}

void Linker::update_symbol_table() {
  for (Symbol& symbol: symbolTable) {
    if (symbol.is_defined == false) continue;
    auto search = sectionIdMap.find(pair<int, int>(symbol.file_id, symbol.section_id));
    if (search != sectionIdMap.end()) {
      symbol.section_id = search->second;
    } else {
      cout << "1-ERROR - no section with id " << symbol.section_id << endl;
      exit(1);
    }

    auto search2 = sectionOffsetMap.find(symbol.section_id);
    if (search2 != sectionOffsetMap.end()) {
      symbol.offset += search2->second;
    } else {
      cout << "ERROR - no section with id " << symbol.section_id << endl;
      exit(1);
    }
  }
  for (int i = 1; i < symbolTable.size(); i++) {
      for (int j = i + 1; j < symbolTable.size(); j++) {
        Symbol symbol1 = symbolTable[i];
        Symbol symbol2 = symbolTable[j];
        if (symbol1.label == symbol2.label && (symbol1.file_id == symbol2.file_id || !symbol1.is_local && !symbol2.is_local && symbol1.is_defined && symbol2.is_defined)) {
          cout << symbol1.label << " " << symbol2.label << endl;
          cout << "Visestruka definicija simbola!" << endl;
          exit(1);
        }
        if (symbol1.label == symbol2.label && symbol1.is_defined && !symbol2.is_defined) {
          symbolTable[j].offset = symbol1.offset;
          symbolTable[j].is_defined = true;
        }
        if (symbol1.label == symbol2.label && !symbol1.is_defined && symbol2.is_defined) {
          symbolTable[i].offset = symbol2.offset;
          symbolTable[i].is_defined = true;
        }
      }
    }
}

void Linker::update_relocation_table() {
  for (Relocation& reloc: relocationTable) {
    auto search = sectionIdMap.find(pair<int, int>(reloc.file_id, reloc.section_id));
    if (search != sectionIdMap.end()) {
      reloc.section_id = search->second;
    } else {
      cout << "ERROR - no section with id " << reloc.section_id << endl;
      exit(1);
    }

    auto search2 = sectionOffsetMap.find(reloc.section_id);
    if (search2 != sectionOffsetMap.end()) {
      reloc.offset += search2->second;
    } else {
      cout << "ERROR - no section with id " << reloc.section_id << endl;
      exit(1);
    }

    search = symbolIdMap.find(pair<int, int>(reloc.file_id, reloc.symbol_id));
    if (search != symbolIdMap.end()) {
      reloc.symbol_id = search->second;
    } else {
      cout << "SYMBOL - no symbol with id " << reloc.symbol_id << endl;
    }
  }
}

void Linker::combine_sections() {
  int counter = 0;

  for (vector<Section>* sectionVector: sectionMap) {
    Section s = sectionVector->front();
    Section newSection(s.section, counter, Section::next_id_combined++, 0, 0);
    for (Section& section: *sectionVector) {
      section.offset = counter;
      sectionOffsetMap.insert(pair<int, int>(section.section_id, counter));
      counter += section.size;
      newSection.size += section.size;
      newSection.data.insert(newSection.data.end(), section.data.begin(), section.data.end());
    }
    sectionTable.push_back(newSection);
    data.insert(data.end(), newSection.data.begin(), newSection.data.end());
  }
}

void Linker::resolve_relocations() {
  for (Relocation& reloc: relocationTable) {
    if (reloc.type == PCREL) {
      int ref_addr = reloc.offset;
      int value = symbolTable[reloc.symbol_id].offset + reloc.addend - ref_addr;
      data[reloc.offset] = value & 0xFF;
      data[reloc.offset + 1] = value >> 8 & 0xFF;
    } else if (reloc.type == ABS) {
      int value = symbolTable[reloc.symbol_id].offset; // + addend??? addend is not needed
      data[reloc.offset] = value & 0xFF;
      data[reloc.offset + 1] = value >> 8 & 0xFF;
    }
  }
}

void Linker::generate_binary_output_file() {
  output_file.open(output_file_path, ios::binary);
  int data_size = data.size();
  output_file.write((char*)&data_size, sizeof(data_size));
  char data_arr[data.size()];
  copy(data.begin(), data.end(), data_arr);
  output_file.write(data_arr, data.size());
  output_file.close();
}

void Linker::generate_text_output_file() {
  output_file_path += ".txt";
  text_file.open(output_file_path);

  int mul = 0;
  for (int i = 0; i < data.size(); i++) {
    if (i % 8 == 0) text_file << hex << mul++ * 8 << ":\t";
    text_file << hex << setfill('0') << setw(2) << ((int)data[i] & 0xFF) << ' ';
    if (i % 8 == 7) text_file << endl;
    else if (i % 4 == 3) text_file << '\t';
  }
  text_file << endl << endl;
  text_file.close();  
}

void Linker::process_data() {
  combine_sections();
  update_symbol_table();
  update_relocation_table();
  resolve_relocations();

  generate_binary_output_file();
  generate_text_output_file();
}

void Linker::print_symbol_table() {
  cout << "TABELA SIMBOLA:" << endl;
  cout << "====================================================" << endl;
  for (int i = 0; i < symbolTable.size(); i++) {
    Symbol curr = symbolTable[i];
    cout << curr.section_id << " " << curr.label << " " << curr.offset << " " << curr.id << " " << curr.is_defined << endl;
  }
  cout << "====================================================" << endl;
}

void Linker::print_relocation_table() {
  cout << "TABELA RELOKACIJA:" << endl;
  cout << "====================================================" << endl;
  for (int i = 0; i < relocationTable.size(); i++) {
    Relocation curr = relocationTable[i];
    cout << curr.section_id << " " << curr.offset << " " << curr.symbol << endl;
  }
  cout << "====================================================" << endl;
}
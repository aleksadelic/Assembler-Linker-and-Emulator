#include "../inc/emulator.h"
#include <iostream>
#include <iomanip>
#include <bitset>

string AddressTypeString[] = { "IMMED", "REGDIR", "REGIND", "REGIND_DIS", "MEMDIR", "REGDIR_DIS" };
string InstructionString[] = { "HALT", "INT", "IRET", "CALL", "RET", "JUMP", "XCHG", "ARITMETIC", "LOGIC", "SHIFT", "LDR", "STR" };
string JumpInstrString[] = { "JMP", "JEQ", "JNE", "JGT" };
string AritmeticInstrString[] = { "ADD", "SUB", "MUL", "DIV", "CMP" };
string LogicInstrString[] = { "NOT", "AND", "OR", "XOR", "TEST" };
string ShiftInstrString[] = { "SHL", "SHR" };
string UpdateModeString[] = { "NONE", "PREDECR", "PREINCR", "POSTDECR", "POSTINCR" };

Emulator::Emulator(string input_file_path) {
  input_file.open(input_file_path, ios::binary);
  int data_size;
  input_file.read((char*)&data_size, sizeof(data_size));
  input_file.read((char*)memory, data_size);
  input_file.close();

  reg[PC] = 0;
  reg[SP] = 0xFF00;
  for (int i = 0; i < 6; i++)
    reg[i] = 0;
  end = false;
}

char Emulator::fetch_byte() {
  return memory[reg[PC]++];
}

pair<char, char> Emulator::fetch_instruction_code() {
  char instr = fetch_byte();
  char op_code = instr >> 4 & 0xF;
  char instr_mod = instr & 0xF;
  return pair<char, char>(op_code, instr_mod);
}

pair<char, char> Emulator::fetch_regs_descr() {
  char descr = fetch_byte();
  int src = descr & 0xF;
  int dst = descr >> 4;
  return pair<char, char>(dst, src);
}

pair<char, char> Emulator::fetch_addr_mode() {
  char addr_mode = fetch_byte();
  char mode = addr_mode & 0xF;
  char update = addr_mode >> 4;
  return pair<int, int>(update, mode);
}

short Emulator::fetch_operand() {
  char low = fetch_byte();
  char high = fetch_byte();
  short opr = (short)high << 8 | (low & 0xFF);
  return opr;
}

short Emulator::fetch_operand_memory(uint16_t addr) {
  return (memory[addr] & 0xFF) | ((short)memory[addr + 1] << 8);
}

short Emulator::determine_operand(int regS) {
  short operand;
  pair<char, char> addr_mode = fetch_addr_mode();
  char update = addr_mode.first;
  char mode = addr_mode.second;
  switch (mode) {
  case REGDIR:
  {
    operand = reg[regS];
    break;
  }
  case REGIND:
  {
    reg[regS] += update_reg_pre(update);
    uint16_t addr = reg[regS];
    operand = fetch_operand_memory(addr);
    reg[regS] += update_reg_post(update);
    break;
  }
  case IMMED:
  {
    operand = fetch_operand();
    break;
  }
  case MEMDIR:
  {
    uint16_t addr = fetch_operand();
    operand = fetch_operand_memory(addr);
    break;
  }
  case REGIND_DIS:
  {
    reg[regS] += update_reg_pre(update);
    short dis = fetch_operand();
    operand = fetch_operand_memory((uint16_t)(reg[regS] + dis));
    reg[regS] += update_reg_post(update);
    break;
  }
  case REGDIR_DIS:
  {
    int dis = fetch_operand();
    operand = reg[regS] + dis;
    break;
  }
  default:
  {
    cout << "Nepoznat tip adresiranja!" << endl;
    handle_exception();
  }
  }

  return operand;
}

void Emulator::determine_store_location(int regS, int regD) {
  uint16_t operand;
  pair<char, char> addr_mode = fetch_addr_mode();
  char update = addr_mode.first;
  char mode = addr_mode.second;
  switch (mode) {
  case REGDIR:
  {
    reg[regS] = reg[regD];
    break;
  }
  case REGIND:
  {
    reg[regS] += update_reg_pre(update);
    memory[(uint16_t)reg[regS]] = reg[regD] & 0xFF;
    memory[(uint16_t)(reg[regS] + 1)] = (reg[regD] >> 8) & 0xFF;
    reg[regS] += update_reg_post(update);
    break;
  }
  case IMMED:
  {
    cout << "Neispravna instrukcija!" << endl;
    handle_exception();
    break;
  }
  case MEMDIR:
  {
    uint16_t addr = fetch_operand();
    memory[addr] = reg[regD] & 0xFF;
    memory[addr + 1] = (reg[regD] >> 8) & 0xFF;
    break;
  }
  case REGIND_DIS:
  {
    short dis = fetch_operand();
    reg[regS] += update_reg_pre(update);
    memory[(uint16_t)(reg[regS] + dis)] = reg[regD] & 0xFF;
    memory[(uint16_t)(reg[regS] + dis + 1)] = (reg[regD] >> 8) & 0xFF;
    reg[regS] += update_reg_post(update);
    break;
  }
  case REGDIR_DIS:
  {
    cout << "Neispravna instrukcija!" << endl;
    handle_exception();
    break;
  }
  default:
  {
    cout << "Nepoznat tip adresiranja!" << endl;
    handle_exception();
  }
  }
  
}

short Emulator::update_reg_pre(char mode) {
  switch (mode) {
  case PREINCR:
    return 2;
  case PREDECR:
    return -2;
  default:
    return 0;
  }
}

short Emulator::update_reg_post(char mode) {
  switch (mode) {
  case POSTINCR:
    return 2;
  case POSTDECR:
    return -2;
  default:
    return 0;
  }
}

void Emulator::stack_push(short value) {
  reg[SP] -= 2;
  char low = value & 0xFF;
  char high = value >> 8 & 0xFF;
  memory[(uint16_t)reg[SP]] = low;
  memory[(uint16_t)reg[SP] + 1] = high;
}

short Emulator::stack_pop() {
  char low = memory[(uint16_t)reg[SP]];
  char high = memory[(uint16_t)reg[SP] + 1];
  reg[SP] += 2;
  short ret = (short)high << 8 | (low & 0xFF);
  return ret;
}

void Emulator::emulate() {
  pair<char, char> instr;
  char op_code = 0;
  char instr_mod = 0;
  reg[PC] = (short)memory[1] << 8 | memory[0] & 0xFF; 
  while (!end) {
    instr = fetch_instruction_code();
    op_code = instr.first;
    instr_mod = instr.second;
    switch (op_code) {
    case HALT:
      process_halt();
      break;
    case INT:
      process_int();
      break;
    case IRET:
      process_iret();
      break;
    case CALL:
      process_call();
      break;
    case RET:
      process_ret();
      break;
    case JUMP:
      process_jump(instr_mod);
      break;
    case XCHG:
      process_xchg();
      break;
    case ARITMETIC:
      process_aritmetic(instr_mod);
      break;
    case LOGIC:
      process_logic(instr_mod);
      break;
    case SHIFT:
      process_shift(instr_mod);
      break;
    case LDR:
      process_ldr();
      break;
    case STR:
      process_str();
      break;
    default:
      cout << "Nepoznata instrukcija!" << endl;
      handle_exception();
    }
    /*cout << "\tPC=" << reg[PC] << " SP=" << hex << (uint16_t)reg[SP];
    for (int i = 0; i < 6; i++) {
      cout << " r" << i << "=" << reg[i]; 
    }
    cout << " stack:";
    for (int i = 65277; i >= 65270; i--) {
      cout << " " << hex << setfill('0') << setw(2) << ((int)memory[i] & 0xFF);
    }
    cout << endl;*/
  }
  print_processor_state();
}

void Emulator::process_halt() {
  end = true;
}

void Emulator::process_int() {
  int regD = fetch_regs_descr().first;
  stack_push(reg[PC]);
  stack_push(psw.psw_value);
  int entry = reg[regD];
  reg[PC] = (short)memory[entry * 2 + 1] << 8 | (memory[entry * 2] & 0xFF);
}

void Emulator::process_iret() {
  psw.psw_value = stack_pop();
  reg[PC] = stack_pop();
}

void Emulator::process_call() {
  int regS = fetch_regs_descr().second;
  int operand = determine_operand(regS);
  stack_push(reg[PC]);
  reg[PC] = operand;
}

void Emulator::process_ret() {
  reg[PC] = stack_pop();
}

void Emulator::process_jump(char instr_mod) {
  int regS = fetch_regs_descr().second;
  bool cond = false;
  switch (instr_mod) {
  case JMP:
    cond = true;
    break;
  case JEQ:
    cond = psw.getZ();
    break;
  case JNE:
    cond = !psw.getZ();
    break;
  case JGT:
    cond = !psw.getZ() && (psw.getO() == psw.getN()); 
    break;
  default:
    cout << "Nepoznata instrukcija" << endl;
    handle_exception();
  }
  if (!cond)
    return;

  int operand = determine_operand(regS);
  reg[PC] = operand;
}

void Emulator::process_xchg() {
  pair<char, char> regs_descr = fetch_regs_descr();
  char regD = regs_descr.first;
  char regS = regs_descr.second;

  int temp = reg[regD];
  reg[regD] = reg[regS];
  reg[regS] = temp;
}

void Emulator::process_aritmetic(char instr_mod) {
  pair<char, char> regs_descr = fetch_regs_descr();
  char regD = regs_descr.first;
  char regS = regs_descr.second;

  switch (instr_mod) {
  case ADD:
    reg[regD] += reg[regS];
    break;
  case SUB:
    reg[regD] -= reg[regS];
    break;
  case MUL:
    reg[regD] *= reg[regS];
    break;
  case DIV:
    if (reg[regS] == 0) {
      cout << "Deljenje nulom!" << endl;
      handle_exception();
      break;
    }
    reg[regD] /= reg[regS];
    break;
  case CMP:
  {
    int temp = reg[regD] - reg[regS];
    psw.setZ(temp);
    psw.setN(temp);
    psw.setO(reg[regD], reg[regS], temp);
    psw.setC(reg[regD], reg[regS], instr_mod);
    break;
  }
  default:
    cout << "Nepoznata instrukcija!" << endl;
    handle_exception();
  }

}

void Emulator::process_logic(char instr_mod) {
  pair<char, char> regs_descr = fetch_regs_descr();
  char regD = regs_descr.first;
  char regS = regs_descr.second;

  switch (instr_mod) {
  case NOT:
    reg[regD] = ~reg[regD];
    break;
  case AND:
    reg[regD] &= reg[regS];
    break;
  case OR:
    reg[regD] |= reg[regS];
    break;
  case XOR:
    reg[regD] ^= reg[regS];
    break;
  case TEST:
  {
    short temp = reg[regD] & reg[regS];
    psw.setZ(temp);
    psw.setN(temp);
    break;
  }
  default:
    cout << "Nepoznata instrukcija!" << endl;
    handle_exception();
  }
}

void Emulator::process_shift(char instr_mod) {
  pair<char, char> regs_descr = fetch_regs_descr();
  char regD = regs_descr.first;
  char regS = regs_descr.second;

  switch (instr_mod) {
  case SHL:
  psw.setC(reg[regD], reg[regS], instr_mod);
  reg[regD] <<= reg[regS];
  psw.setZ(reg[regD]);
  psw.setN(reg[regD]);
    break;
  case SHR:
  psw.setC(reg[regD], reg[regS], instr_mod);
  reg[regD] >>= reg[regS];
  psw.setZ(reg[regD]);
  psw.setN(reg[regD]);
    break;
  default:
    cout << "Nepoznata instrukcija!" << endl;
    handle_exception();
  }
}

void Emulator::process_ldr() {
  pair<char, char> regs_descr = fetch_regs_descr();
  char regD = regs_descr.first;
  char regS = regs_descr.second;
  short operand = determine_operand(regS);

  reg[regD] = operand;
}

void Emulator::process_str() {
  pair<char, char> regs_descr = fetch_regs_descr();
  char regD = regs_descr.first;
  char regS = regs_descr.second;
  determine_store_location(regS, regD);
}

bool Emulator::PSW::getZ() {
  return psw_value & Z;
}

bool Emulator::PSW::getO() {
  return psw_value & O;
}

bool Emulator::PSW::getC() {
  return psw_value & C;
}

bool Emulator::PSW::getN() {
  return psw_value & N;
}

void Emulator::PSW::setZ(short val) {
  if (val == 0) 
    psw_value |= Z;
  else
    psw_value &= ~Z;
}

void Emulator::PSW::setO(short dst_val, short src_val, short res) {
  if ((dst_val > 0 && src_val < 0 && res < 0) || (dst_val < 0 && src_val > 0 && res > 0))
    psw_value |= O;
  else
    psw_value &= ~O;
}

void Emulator::PSW::setC(short dst_val, short src_val, short instr_mod) {
  switch (instr_mod) {
  case CMP:
    if (dst_val < src_val) {
      psw_value |= C;
    } else {
      psw_value &= ~C;
    }
    break;
  case SHL:
    if (src_val < 16 && ((dst_val >> (16 - src_val)) & 1)) {
      psw_value |= C;
    } else {
      psw_value &= ~C;
    }
    break;
  case SHR:
    if (dst_val >> (src_val - 1) & 1) {
      psw_value |= C;
    } else {
      psw_value &= ~C;
    }
  }
}

void Emulator::PSW::setN(short val) {
  if (val < 0) 
    psw_value |= N;
  else
    psw_value &= ~N;
}

void Emulator::handle_exception() {
  stack_push(reg[PC]);
  stack_push(psw.psw_value);
  int entry = 1;
  psw.psw_value |= PSW::PSWFlags::I;
  reg[PC] = (short)memory[entry * 2 + 1] << 8 | (memory[entry * 2] & 0xFF);
}

void Emulator::print_processor_state() {
  cout << "------------------------------------------------" << endl;
  cout << "Emulated processor executed halt instruction" << endl;
  cout << "Emulated processor state: ";
  cout << "psw=0b" << bitset<16>(psw.psw_value) << endl;
  for (int i = 0; i <= 7; i++) {
    cout << "r" << i << "=0x" << hex << setfill('0') << setw(4) << reg[i];
    if (i % 4 == 3) cout << endl;
    else cout << '\t';
  }
  cout << memory[0x7F00] << endl;
}
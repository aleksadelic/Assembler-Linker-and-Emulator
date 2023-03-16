#include <fstream>
#include <vector>
#include <utility>

using namespace std;

class Emulator {
public:

  enum AddressType { IMMED, REGDIR, REGIND, REGIND_DIS, MEMDIR, REGDIR_DIS };
  enum Instruction { HALT, INT, IRET, CALL, RET, JUMP, XCHG, ARITMETIC, LOGIC, SHIFT, LDR, STR };
  enum JumpInstr { JMP, JEQ, JNE, JGT };
  enum AritmeticInstr { ADD, SUB, MUL, DIV, CMP };
  enum LogicInstr { NOT, AND, OR, XOR, TEST };
  enum ShiftInstr { SHL, SHR };
  enum UpdateMode { NONE, PREDECR, PREINCR, POSTDECR, POSTINCR };
  enum SpecRegs { SP = 6, PC = 7 };

  Emulator(string input_file_path);

  char fetch_byte();
  pair<char, char> fetch_instruction_code();
  pair<char, char> fetch_regs_descr();
  pair<char, char> fetch_addr_mode();
  short fetch_operand();
  void emulate();

  void process_halt();
  void process_int();
  void process_iret();
  void process_call();
  void process_ret();
  void process_jump(char instr_mod);
  void process_xchg();
  void process_aritmetic(char instr_mod);
  void process_logic(char instr_mod);
  void process_shift(char instr_mod);
  void process_ldr();
  void process_str();

  void stack_push(short value);
  short stack_pop();
  short fetch_operand_memory(uint16_t address);
  short determine_operand(int regS);
  void determine_store_location(int regS, int regD);
  short update_reg_pre(char mode);
  short update_reg_post(char mode);
  void print_processor_state();
  void handle_exception();

  struct PSW {
    enum PSWFlags { Z = 1, O = 2, C = 4, N = 8, Tr = 1 << 13, Tl = 1 << 14, I = 1 << 15};
    short psw_value = 0;

    bool getZ();
    bool getO();
    bool getC();
    bool getN();

    void setZ(short val);
    void setO(short dst_val, short src_val, short res);
    void setC(short dst_val, short src_val, short instr_mod);
    void setN(short val);
  };
  
private:
  ifstream input_file;
  char memory[1<<16];
  short reg[8];
  bool end;
  PSW psw;
};
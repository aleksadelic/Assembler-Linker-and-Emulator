%{
  #include "./inc/assembler.h"

  extern Assembler* assembler;
%}
%option noyywrap

digit [0-9]
letter [a-zA-Z_]
char [a-zA-Z0-9_]
newLine \n|\r|\r\n

symbol {letter}{char}*
literal -?[0-9]+|0x[0-9A-F]+
symbol_or_literal {symbol}|{literal}
reg r[0-7]|psw
operand ${literal}|${symbol}|{literal}|{symbol}|%{symbol}|{reg}|\[{reg}\]|\[{reg}\ +\ {literal}\]|\[{reg}\ +\ {symbol}\]
comma_and_symbol ,\ +{symbol_or_literal}

directive \.{symbol}\ *{symbol_or_literal}*{comma_and_symbol}*
no_arg_ins halt|iret|ret
one_arg_reg_ins int|push|pop|not
two_arg_reg_ins xchg|add|sub|mul|div|cmp|and|or|xor|test|shl|shr
one_opr_ins call|jmp|jeq|jne|jgt
reg_opr_ins ldr|str

%%
[\t\n\ ]          ;
#.* ;
{directive}   { assembler->process_directive(yytext); }
{symbol}: { assembler->process_label(yytext); }

{no_arg_ins} { assembler->process_instruction_with_no_arguments(yytext); }
{one_arg_reg_ins}\ +{reg} { assembler->process_register_instruction_with_one_argument(yytext); }
{two_arg_reg_ins}\ +{reg},\ +{reg} { assembler->process_register_instruction_with_two_arguments(yytext); }

{one_opr_ins}\ +{literal} { assembler->process_one_operand_instruction(yytext, Assembler::AddressType::IMMED); }
{one_opr_ins}\ +{symbol} { assembler->process_one_operand_instruction(yytext, Assembler::AddressType::IMMED); }
{one_opr_ins}\ +%{symbol} { assembler->process_one_operand_instruction(yytext, Assembler::AddressType::REGDIR_PCREL); }
{one_opr_ins}\ +\*{literal} { assembler->process_one_operand_instruction(yytext, Assembler::AddressType::MEMDIR); }
{one_opr_ins}\ +\*{reg} { assembler->process_one_operand_instruction(yytext, Assembler::AddressType::REGDIR); }
{one_opr_ins}\ +\*{symbol} { assembler->process_one_operand_instruction(yytext, Assembler::AddressType::MEMDIR); }
{one_opr_ins}\ +\*\[{reg}\] { assembler->process_one_operand_instruction(yytext, Assembler::AddressType::REGIND); }
{one_opr_ins}\ +\*\[{reg}\ \+\ {literal}\] { assembler->process_one_operand_instruction(yytext, Assembler::AddressType::REGIND_DIS); }
{one_opr_ins}\ +\*\[{reg}\ \+\ {symbol}\] { assembler->process_one_operand_instruction(yytext, Assembler::AddressType::REGIND_DIS); }

{reg_opr_ins}\ +{reg},\ +${literal} { assembler->process_register_operand_instruction(yytext, Assembler::AddressType::IMMED); }
{reg_opr_ins}\ +{reg},\ +${symbol} { assembler->process_register_operand_instruction(yytext, Assembler::AddressType::IMMED); }
{reg_opr_ins}\ +{reg},\ +{reg} { assembler->process_register_operand_instruction(yytext, Assembler::AddressType::REGDIR); }
{reg_opr_ins}\ +{reg},\ +{literal} { assembler->process_register_operand_instruction(yytext, Assembler::AddressType::MEMDIR); }
{reg_opr_ins}\ +{reg},\ +{symbol} { assembler->process_register_operand_instruction(yytext, Assembler::AddressType::MEMDIR); }
{reg_opr_ins}\ +{reg},\ +%{symbol} { assembler->process_register_operand_instruction(yytext, Assembler::AddressType::REGIND_PCREL); }
{reg_opr_ins}\ +{reg},\ +\[{reg}\] { assembler->process_register_operand_instruction(yytext, Assembler::AddressType::REGIND); }
{reg_opr_ins}\ +{reg},\ +\[{reg}\ \+\ {literal}\] { assembler->process_register_operand_instruction(yytext, Assembler::AddressType::REGIND_DIS); }
{reg_opr_ins}\ +{reg},\ +\[{reg}\ \+\ {symbol}\] { assembler->process_register_operand_instruction(yytext, Assembler::AddressType::REGIND_DIS); }
.                { cout << "ERROR!" << endl; exit(1); }
%%
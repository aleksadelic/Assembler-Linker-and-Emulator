#!/bin/bash

flex ./misc/flex.l
g++ ./inc/tableEntryAsm.h ./src/tableEntryAsm.cpp ./inc/assembler.h ./src/assembler.cpp lex.yy.c ./src/mainAsm.cpp -o assembler
g++ ./inc/tableEntryLin.h ./src/tableEntryLin.cpp ./inc/linker.h ./src/linker.cpp ./src/mainLin.cpp -o linker
g++ ./inc/emulator.h ./src/emulator.cpp ./src/mainEmu.cpp -o emulator



#ifndef GEMU_INSTRUCTIONS_H
#define GEMU_INSTRUCTIONS_H

#include "cpu.h"
#include "stdinc.h"

void Cpu_execute(Cpu *cpu, Memory *mem, u8 opcode);

#endif

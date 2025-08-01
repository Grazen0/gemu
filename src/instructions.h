#ifndef GEMU_INSTRUCTIONS_H
#define GEMU_INSTRUCTIONS_H

#include "cpu.h"
#include "stdinc.h"

void Cpu_execute(Cpu* restrict cpu, Memory* restrict mem, u8 opcode);

#endif

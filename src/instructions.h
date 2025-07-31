#ifndef GEMU_INSTRUCTIONS_H
#define GEMU_INSTRUCTIONS_H

#include "cpu.h"
#include <stdint.h>

void Cpu_execute(Cpu* restrict cpu, Memory* restrict mem, uint8_t opcode);

#endif

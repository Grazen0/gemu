#ifndef CORE_CPU_INSTRUCTIONS_H
#define CORE_CPU_INSTRUCTIONS_H

#include "core/cpu/cpu.h"
#include <stdint.h>

void Cpu_execute(Cpu* restrict cpu, Memory* restrict mem, uint8_t opcode);

#endif

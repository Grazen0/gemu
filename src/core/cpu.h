#ifndef CORE_CPU_H
#define CORE_CPU_H

#include <stdbool.h>
#include <stdint.h>

typedef enum CpuFlag {
    CPU_FLAG_C = 1 << 0,
    CPU_FLAG_N = 1 << 1,
    CPU_FLAG_P = 1 << 2,
    CPU_FLAG_H = 1 << 4,
    CPU_FLAG_Z = 1 << 6,
    CPU_FLAG_S = 1 << 7,
} CpuFlag;

typedef enum CpuTableR {
    CPU_R_B = 0,
    CPU_R_C = 1,
    CPU_R_D = 2,
    CPU_R_E = 3,
    CPU_R_H = 4,
    CPU_R_L = 5,
    CPU_R_HL = 6,
    CPU_R_A = 7,
} CpuTableR;

typedef enum CpuTableRp {
    CPU_RP_BC = 0,
    CPU_RP_DE = 1,
    CPU_RP_HL = 2,
    CPU_RP_SP = 3,
} CpuTableRp;

typedef enum CpuTableRp2 {
    CPU_RP2_BC = 0,
    CPU_RP2_DE = 1,
    CPU_RP2_HL = 2,
    CPU_RP2_AF = 3,
} CpuTableRp2;

typedef enum CpuTableCc {
    CPU_CC_NZ = 0,
    CPU_CC_Z = 1,
    CPU_CC_NC = 2,
    CPU_CC_C = 3,
} CpuTableCc;

typedef enum CpuTableAlu {
    CPU_ALU_ADD = 0,
    CPU_ALU_ADC = 1,
    CPU_ALU_SUB = 2,
    CPU_ALU_SBC = 3,
    CPU_ALU_AND = 4,
    CPU_ALU_XOR = 5,
    CPU_ALU_OR = 6,
    CPU_ALU_CP = 7,
} CpuTableAlu;

typedef enum CpuTableRot {
    CPU_ROT_RLC = 0,
    CPU_ROT_RRC = 1,
    CPU_ROT_RL = 2,
    CPU_ROT_RR = 3,
    CPU_ROT_SLA = 4,
    CPU_ROT_SRA = 5,
    CPU_ROT_SWAP = 6,
    CPU_ROT_SRL = 7,
} CpuTableRot;

typedef struct Memory {
    void* ctx;
    uint8_t (*read)(const void* ctx, uint16_t addr);
    void (*write)(void* ctx, uint16_t addr, uint8_t value);
} Memory;

typedef struct Cpu {
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint8_t a;
    uint8_t f;
    uint16_t sp;
    uint16_t pc;
    bool halted;
    bool ime;
    int cycle_count;
} Cpu;

Cpu Cpu_new(void);

void Cpu_set_flag(Cpu* restrict cpu, CpuFlag flag, bool value);

bool Cpu_read_cc(const Cpu* restrict cpu, CpuTableCc cc);

uint16_t Cpu_read_rp(const Cpu* restrict cpu, CpuTableRp rp);

void Cpu_write_rp(Cpu* restrict cpu, CpuTableRp rp, uint16_t value);

uint16_t Cpu_read_rp2(const Cpu* restrict cpu, CpuTableRp rp);

void Cpu_write_rp2(Cpu* restrict cpu, CpuTableRp rp, uint16_t value);

void Cpu_alu(Cpu* restrict cpu, CpuTableAlu alu, uint8_t rhs);

uint8_t Cpu_read_mem(Cpu* restrict cpu, const Memory* restrict mem, uint16_t addr);

uint16_t Cpu_read_mem_u16(Cpu* restrict cpu, const Memory* restrict mem, uint16_t addr);

void Cpu_write_mem(Cpu* restrict cpu, Memory* restrict mem, uint16_t addr, uint8_t value);

void Cpu_write_mem_u16(Cpu* restrict cpu, Memory* restrict mem, uint16_t addr, uint16_t value);

uint8_t Cpu_read_pc(Cpu* restrict cpu, const Memory* restrict mem);

uint16_t Cpu_read_pc_u16(Cpu* restrict cpu, const Memory* restrict mem);

uint8_t Cpu_read_r(Cpu* restrict cpu, const Memory* restrict mem, CpuTableR r);

void Cpu_write_r(Cpu* restrict cpu, Memory* mem, CpuTableR r, uint8_t value);

void Cpu_stack_push_u16(Cpu* restrict cpu, Memory* restrict mem, uint16_t value);

uint16_t Cpu_stack_pop_u16(Cpu* restrict cpu, Memory* restrict mem);

void Cpu_tick(Cpu* restrict cpu, Memory* restrict mem);

void Cpu_execute(Cpu* restrict cpu, Memory* restrict mem, uint8_t opcode);

void Cpu_execute_prefixed(Cpu* restrict cpu, Memory* restrict mem, uint8_t opcode);

void Cpu_interrupt(Cpu* restrict cpu, Memory* restrict mem, uint8_t handler_location);

#endif

#ifndef GEMU_CPU_H
#define GEMU_CPU_H

#include "stdinc.h"

typedef enum : u8 {
    CpuFlag_C = 1 << 4,
    CpuFlag_H = 1 << 5,
    CpuFlag_N = 1 << 6,
    CpuFlag_Z = 1 << 7,
} CpuFlag;

typedef enum : u8 {
    CpuTableR_B = 0,
    CpuTableR_C = 1,
    CpuTableR_D = 2,
    CpuTableR_E = 3,
    CpuTableR_H = 4,
    CpuTableR_L = 5,
    CpuTableR_HL = 6,
    CpuTableR_A = 7,
} CpuTableR;

typedef enum : u8 {
    CpuTableRp_BC = 0,
    CpuTableRp_DE = 1,
    CpuTableRp_HL = 2,
    CpuTableRp_SP = 3,
} CpuTableRp;

typedef enum : u8 {
    CpuTableRp2_BC = 0,
    CpuTableRp2_DE = 1,
    CpuTableRp2_HL = 2,
    CpuTableRp2_AF = 3,
} CpuTableRp2;

typedef enum : u8 {
    CpuTableCc_NZ = 0,
    CpuTableCc_Z = 1,
    CpuTableCc_NC = 2,
    CpuTableCc_C = 3,
} CpuTableCc;

typedef enum : u8 {
    CpuTableAlu_Add = 0,
    CpuTableAlu_Adc = 1,
    CpuTableAlu_Sub = 2,
    CpuTableAlu_Sbc = 3,
    CpuTableAlu_And = 4,
    CpuTableAlu_Xor = 5,
    CpuTableAlu_Or = 6,
    CpuTableAlu_Cp = 7,
} CpuTableAlu;

typedef struct {
    void *ctx;
    u8 (*read)(const void *ctx, u16 addr);
    void (*write)(void *ctx, u16 addr, u8 value);
} Memory;

typedef enum : u8 {
    CpuMode_Running,
    CpuMode_Halted,
    CpuMode_Stopped,
} CpuMode;

typedef struct {
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 h;
    u8 l;
    u8 a;
    u8 f;
    u16 sp;
    u16 pc;
    CpuMode mode;
    bool queued_ime;
    bool ime;
    int cycle_count;
} Cpu;

[[nodiscard]] Cpu Cpu_new();

[[nodiscard]] bool Cpu_read_cc(const Cpu *self, CpuTableCc cc);

[[nodiscard]] u16 Cpu_read_rp(const Cpu *self, CpuTableRp rp);

void Cpu_write_rp(Cpu *self, CpuTableRp rp, u16 value);

[[nodiscard]] u16 Cpu_read_rp2(const Cpu *self, CpuTableRp rp);

void Cpu_write_rp2(Cpu *self, CpuTableRp rp, u16 value);

u8 Cpu_read_mem(Cpu *self, const Memory *mem, u16 addr);

u16 Cpu_read_mem_u16(Cpu *self, const Memory *mem, u16 addr);

void Cpu_write_mem(Cpu *self, Memory *mem, u16 addr, u8 value);

void Cpu_write_mem_u16(Cpu *self, Memory *mem, u16 addr, u16 value);

u8 Cpu_read_pc(Cpu *self, const Memory *mem);

u16 Cpu_read_pc_u16(Cpu *self, const Memory *mem);

u8 Cpu_read_r(Cpu *self, const Memory *mem, CpuTableR r);

void Cpu_write_r(Cpu *self, Memory *mem, CpuTableR r, u8 value);

void Cpu_stack_push_u16(Cpu *self, Memory *mem, u16 value);

u16 Cpu_stack_pop_u16(Cpu *self, const Memory *mem);

void Cpu_tick(Cpu *self, Memory *mem);

void Cpu_interrupt(Cpu *self, Memory *mem, u8 handler_location);

#endif

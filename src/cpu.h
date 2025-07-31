#ifndef CORE_CPU_H
#define CORE_CPU_H

#include <stdint.h>

typedef enum CpuFlag : uint8_t {
    CpuFlag_C = 1 << 4,
    CpuFlag_H = 1 << 5,
    CpuFlag_N = 1 << 6,
    CpuFlag_Z = 1 << 7,
} CpuFlag;

typedef enum CpuTableR : uint8_t {
    CpuTableR_B = 0,
    CpuTableR_C = 1,
    CpuTableR_D = 2,
    CpuTableR_E = 3,
    CpuTableR_H = 4,
    CpuTableR_L = 5,
    CpuTableR_HL = 6,
    CpuTableR_A = 7,
} CpuTableR;

typedef enum CpuTableRp : uint8_t {
    CpuTableRp_BC = 0,
    CpuTableRp_DE = 1,
    CpuTableRp_HL = 2,
    CpuTableRp_SP = 3,
} CpuTableRp;

typedef enum CpuTableRp2 : uint8_t {
    CpuTableRp2_BC = 0,
    CpuTableRp2_DE = 1,
    CpuTableRp2_HL = 2,
    CpuTableRp2_AF = 3,
} CpuTableRp2;

typedef enum CpuTableCc : uint8_t {
    CpuTableCc_NZ = 0,
    CpuTableCc_Z = 1,
    CpuTableCc_NC = 2,
    CpuTableCc_C = 3,
} CpuTableCc;

typedef enum CpuTableAlu : uint8_t {
    CpuTableAlu_Add = 0,
    CpuTableAlu_Adc = 1,
    CpuTableAlu_Sub = 2,
    CpuTableAlu_Sbc = 3,
    CpuTableAlu_And = 4,
    CpuTableAlu_Xor = 5,
    CpuTableAlu_Or = 6,
    CpuTableAlu_Cp = 7,
} CpuTableAlu;

typedef struct Memory {
    void* ctx;
    uint8_t (*read)(const void* ctx, uint16_t addr);
    void (*write)(void* ctx, uint16_t addr, uint8_t value);
} Memory;

typedef enum CpuMode {
    CpuMode_Running,
    CpuMode_Halted,
    CpuMode_Stopped,
} CpuMode;

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
    CpuMode mode;
    bool queued_ime;
    bool ime;
    int cycle_count;
} Cpu;

[[nodiscard]] Cpu Cpu_new(void);

[[nodiscard]] bool Cpu_read_cc(const Cpu* restrict cpu, CpuTableCc cc);

[[nodiscard]] uint16_t Cpu_read_rp(const Cpu* restrict cpu, CpuTableRp rp);

void Cpu_write_rp(Cpu* restrict cpu, CpuTableRp rp, uint16_t value);

[[nodiscard]] uint16_t Cpu_read_rp2(const Cpu* restrict cpu, CpuTableRp rp);

void Cpu_write_rp2(Cpu* restrict cpu, CpuTableRp rp, uint16_t value);

uint8_t Cpu_read_mem(Cpu* restrict cpu, const Memory* restrict mem, uint16_t addr);

uint16_t Cpu_read_mem_u16(Cpu* restrict cpu, const Memory* restrict mem, uint16_t addr);

void Cpu_write_mem(Cpu* restrict cpu, Memory* restrict mem, uint16_t addr, uint8_t value);

void Cpu_write_mem_u16(Cpu* restrict cpu, Memory* restrict mem, uint16_t addr, uint16_t value);

uint8_t Cpu_read_pc(Cpu* restrict cpu, const Memory* restrict mem);

uint16_t Cpu_read_pc_u16(Cpu* restrict cpu, const Memory* restrict mem);

uint8_t Cpu_read_r(Cpu* restrict cpu, const Memory* restrict mem, CpuTableR r);

void Cpu_write_r(Cpu* restrict cpu, Memory* mem, CpuTableR r, uint8_t value);

void Cpu_stack_push_u16(Cpu* restrict cpu, Memory* restrict mem, uint16_t value);

uint16_t Cpu_stack_pop_u16(Cpu* restrict cpu, const Memory* restrict mem);

void Cpu_tick(Cpu* restrict cpu, Memory* restrict mem);

void Cpu_interrupt(Cpu* restrict cpu, Memory* restrict mem, uint8_t handler_location);

#endif

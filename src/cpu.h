#ifndef GEMU_CPU_H
#define GEMU_CPU_H

#include "stdinc.h"
#include <stddef.h>

static constexpr u8 CPU_MCYCLE = 4;

typedef struct {
    void *ctx;
    u8 (*read)(const void *ctx, u16 addr);
    void (*write)(void *ctx, u16 addr, u8 value);
} Memory;

typedef enum : u8 {
    MODE_RUNNING,
    MODE_HALTED,
    MODE_STOPPED,
} CpuMode;

typedef struct {
    size_t mcycle_cnt;
    u16 sp;
    u16 pc;
    CpuMode mode;
    u8 b;
    u8 c;
    u8 d;
    u8 e;
    u8 h;
    u8 l;
    u8 a;
    u8 f;
    bool queued_ime;
    bool ime;
} Cpu;

[[nodiscard]] Cpu cpu_init();

u64 cpu_step(Cpu *cpu, Memory *mem);

void cpu_interrupt(Cpu *cpu, Memory *mem, u8 handler_location);

#endif

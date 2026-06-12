#ifndef GEMU_CPU_H
#define GEMU_CPU_H

#include "sink.h"
#include "util.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static constexpr u8 CPU_MCYCLE = 4;

typedef struct {
    u8 (*read)(void *ptr, u16 addr);
    void (*write)(void *ptr, u16 addr, u8 value);
    void (*deinit)(void *ptr);
} MemoryVTable;

typedef struct {
    void *ptr;
    const MemoryVTable *vtable;
} Memory;

static inline u8 mem_read(Memory mem, u16 addr)
{
    return mem.vtable->read(mem.ptr, addr);
}

static inline void mem_write(Memory mem, u16 addr, u8 value)
{
    mem.vtable->write(mem.ptr, addr, value);
}
typedef enum : u8 {
    CPU_MODE_RUNNING,
    CPU_MODE_HALTED,
    CPU_MODE_STOPPED,
} CpuMode;

typedef struct {
    Sink sink; // borrowed
    size_t mcycle_cnt;
    u16 sp;
    u16 pc;
    u16 start_pc;
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

[[nodiscard]] Cpu cpu_init(Sink sink);

void cpu_step(Cpu cpu[static 1], Memory mem);

bool cpu_interrupt(Cpu cpu[static 1], Memory mem, u8 handler_location);

#endif

#ifndef GEMU_CPU_H
#define GEMU_CPU_H

#include "util.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static constexpr u8 CPU_MCYCLE = 4;

typedef struct {
    u8 (*read)(void *ptr, u16 addr);
    void (*write)(void *ptr, u16 addr, u8 value);
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

typedef struct {
    void (*vlog)(void *ptr, const char format[], va_list args);
    void (*deinit)(void *ptr);
} LoggerVTable;

typedef struct {
    void *ptr;
    const LoggerVTable *vtable;
} Logger;

static inline void logger_vlog(Logger logger, const char format[], va_list args)
{
    logger.vtable->vlog(logger.ptr, format, args);
}

static inline void logger_deinit(Logger logger)
{
    logger.vtable->deinit(logger.ptr);
}

static inline void logger_box_deinit(Logger *logger)
{
    logger_deinit(*logger);
    free(logger->ptr);
    logger->ptr = nullptr;
}

[[gnu::format(printf, 2, 3)]] static inline void
logger_log(Logger logger, const char format[], ...)
{
    va_list args;
    va_start(args, format);
    logger_vlog(logger, format, args);
    va_end(args);
}

typedef enum : u8 {
    CPU_MODE_RUNNING,
    CPU_MODE_HALTED,
    CPU_MODE_STOPPED,
} CpuMode;

typedef struct {
    Logger logger; // borrowed
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

[[nodiscard]] Cpu cpu_init(Logger logger);

void cpu_step(Cpu *cpu, Memory mem);

void cpu_interrupt(Cpu *cpu, Memory mem, u8 handler_location);

#endif

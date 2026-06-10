#ifndef GEMU_MAPPER_H
#define GEMU_MAPPER_H

#include "util.h"
#include <stddef.h>

typedef struct {
    void (*deinit)(void *ptr);
    u8 (*read)(void *ptr, const u8 *rom, size_t rom_len, u16 addr);
    void (*write)(void *ptr, u16 addr, u8 value);
} MapperVTable;

typedef struct {
    void *ptr;
    const MapperVTable *vtable;
} Mapper;

Mapper mapper_default();

Mapper mapper_from_rom(const u8 *rom, size_t rom_len);

void mapper_box_deinit(Mapper *mapper);

static inline void mapper_deinit(Mapper mapper)
{
    mapper.vtable->deinit(mapper.ptr);
}

static inline u8 mapper_read(Mapper mapper, const u8 *rom, size_t rom_len,
                             u16 addr)
{
    return mapper.vtable->read(mapper.ptr, rom, rom_len, addr);
}

static inline void mapper_write(Mapper mapper, u16 addr, u8 value)
{
    mapper.vtable->write(mapper.ptr, addr, value);
}

#endif

#ifndef GEMU_MAPPER_H
#define GEMU_MAPPER_H

#include "util.h"
#include <stddef.h>

typedef struct MapperVTable MapperVTable;

typedef struct {
    void *ptr;
    const MapperVTable *vtable;
} Mapper;

Mapper mapper_default();

Mapper mapper_from_rom(const u8 *rom, size_t rom_len);

u8 mapper_read(const Mapper *mapper, const u8 *rom, size_t rom_len, u16 addr);

void mapper_write(Mapper *mapper, u16 addr, u8 value);

void mapper_deinit(Mapper *mapper);

void mapper_destroy(Mapper *mapper);

#endif

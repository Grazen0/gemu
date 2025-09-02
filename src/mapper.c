#include "mapper.h"
#include "data.h"
#include "log.h"
#include "macros.h"
#include "stdinc.h"
#include <stddef.h>
#include <stdlib.h>

typedef struct {
    u8 (*read)(const Mapper *mapper, const u8 *rom, size_t rom_len, u16 addr);
    void (*write)(Mapper *mapper, u16 addr, u8 value);
    void (*destroy)(Mapper *mapper);
} MapperInterface;

struct Mapper {
    const MapperInterface *const vtable;
};

typedef struct {
    Mapper base;
} NoMbcMapper;

typedef struct {
    Mapper base;
    size_t rom_banks;
    size_t rom_bank_num;
    u8 banking_mode_sel;
} Mbc1Mapper;

static u8 NoMbcMapper_read([[maybe_unused]] const Mapper *const base,
                           const u8 *const rom, const size_t rom_len,
                           const u16 addr)
{
    if (addr < rom_len)
        return rom[addr];

    return 0xFF;
}

static void NoMbcMapper_write([[maybe_unused]] Mapper *const base,
                              [[maybe_unused]] const u16 addr,
                              [[maybe_unused]] const u8 value)
{
}

static void NoMbcMapper_destroy([[maybe_unused]] Mapper *const base)
{
}

static NoMbcMapper NoMbcMapper_new()
{
    static const MapperInterface vtable = {
        .read = NoMbcMapper_read,
        .write = NoMbcMapper_write,
        .destroy = NoMbcMapper_destroy,
    };

    return (NoMbcMapper){.base = {.vtable = &vtable}};
}

static u8 Mbc1Mapper_read(const Mapper *const base, const u8 *const rom,
                          const size_t rom_len, const u16 addr)
{
    const Mbc1Mapper *const self = (const void *)base;

    if (self->banking_mode_sel == 1)
        log_error("read from $%04X, mode = %i", addr, self->banking_mode_sel);

    if (addr < 0x4000) // 0000-3FFF (ROM bank X0)
        return rom[addr];

    if (addr < 0x8000) // 4000-7FFF (ROM bank 01-7F)
        return rom[(0x4000 * self->rom_bank_num) + addr - 0x4000];

    return 0xFF;
}

static void Mbc1Mapper_write(Mapper *const base, const u16 addr, const u8 value)
{
    Mbc1Mapper *const self = (void *)base;

    if (addr >= 0x2000 && addr < 0x4000) {
        // 2000-3FFF (ROM bank number)
        self->rom_bank_num = (value & 0x11111) % self->rom_banks;

        if (self->rom_bank_num == 0)
            self->rom_bank_num = 1;
    } else if (addr < 0x6000) {
        // 4000-5FFF (upper bits of ROM bank number)
    } else if (addr < 0x8000) {
        // 6000-7FFF (banking mode select)
        if (self->rom_banks >= 64)
            self->banking_mode_sel = value & 1;
    }
}

static void Mbc1Mapper_destroy([[maybe_unused]] Mapper *const base)
{
}

static Mbc1Mapper Mbc1Mapper_new(const u8 rom_size_code, const u8 ram_size_code)
{
    static const MapperInterface vtable = {
        .read = Mbc1Mapper_read,
        .write = Mbc1Mapper_write,
        .destroy = Mbc1Mapper_destroy,
    };

    BAIL_IF(ram_size_code != 0);

    return (Mbc1Mapper){
        .base = {.vtable = &vtable},
        .rom_banks = rom_banks_from_size_code(rom_size_code),
        .banking_mode_sel = 0,
        .rom_bank_num = 0,
    };
}

#define RETURN_BOX(data)                                \
    do {                                                \
        typeof(data) *const ptr = malloc(sizeof(*ptr)); \
        BAIL_IF_NULL(ptr);                              \
                                                        \
        const typeof(data) data_value = data;           \
        memcpy(ptr, &data_value, sizeof(*ptr));         \
        return &ptr->base;                              \
    } while (0)

Mapper *Mapper_from_rom(const u8 *rom, const size_t rom_len)
{
    BAIL_IF(rom_len < 0x8000);

    const u8 ctype = rom[RomHeader_CartridgeType];
    const u8 rom_size_code = rom[RomHeader_RomSize];
    const u8 ram_size_code = rom[RomHeader_RamSize];

    BAIL_IF(ram_size_code == 1, "invalid RAM size");

    switch (ctype) {
    case CartridgeType_RomOnly:
        RETURN_BOX(NoMbcMapper_new());
    case CartridgeType_Mbc1:
        RETURN_BOX(Mbc1Mapper_new(rom_size_code, ram_size_code));
    default:
        BAIL("unimplemented mapper: $%02X", ctype);
    }
}

u8 Mapper_read(const Mapper *const self, const u8 *rom, const size_t rom_len,
               u16 addr)
{
    BAIL_IF(addr >= 0x8000 && (addr < 0xA000 || addr >= 0xC000),
            "unexpected mapper read (addr = $%04X)", addr);

    return self->vtable->read(self, rom, rom_len, addr);
}

void Mapper_write(Mapper *const self, const u16 addr, const u8 value)
{
    self->vtable->write(self, addr, value);
}

void Mapper_destroy(Mapper *const self)
{
    if (self != nullptr) {
        self->vtable->destroy(self);
        free(self);
    }
}

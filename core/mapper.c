#include "mapper.h"
#include "data.h"
#include "log.h"
#include "macros.h"
#include "stdinc.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

struct MapperVTable {
    void (*deinit)(void *ptr);
    u8 (*read)(const void *ptr, const u8 *rom, size_t rom_len, u16 addr);
    void (*write)(void *ptr, u16 addr, u8 value);
};

static u8 no_mbc_read([[maybe_unused]] const void *ptr, const u8 *rom,
                      size_t rom_len, u16 addr)
{
    if (addr < rom_len)
        return rom[addr];

    return 0xFF;
}

static void no_mbc_write([[maybe_unused]] void *ptr, [[maybe_unused]] u16 addr,
                         [[maybe_unused]] u8 value)
{
}

static void no_mbc_deinit([[maybe_unused]] void *ptr)
{
}

typedef struct {
    size_t rom_banks;
    u8 rom_bank_num;
    u8 banking_mode;
} Mbc1Mapper;

static void mbc1_deinit([[maybe_unused]] void *ptr)
{
}

static Mbc1Mapper mbc1_init(u8 rom_size_code, u8 ram_size_code)
{
    if (ram_size_code != 0)
        BAIL();

    return (Mbc1Mapper){
        .rom_banks = rom_banks_from_size_code(rom_size_code),
        .banking_mode = 0,
        .rom_bank_num = 0,
    };
}

static u8 mbc1_read(const void *ptr, const u8 *rom, size_t rom_len, u16 addr)
{
    const Mbc1Mapper *mapper = ptr;

    if (mapper->banking_mode == 1)
        log_error("read from $%04X, mode = %i", addr, mapper->banking_mode);

    if (addr < 0x4000) { // 0000-3FFF (ROM bank X0)
        assert(addr <= rom_len);
        return rom[addr];
    }

    if (addr < 0x8000) { // 4000-7FFF (ROM bank 01-7F)
        size_t bank_num = mapper->rom_bank_num;
        if (bank_num == 0)
            bank_num = 1;

        u16 phys_addr = (0x4000 * bank_num) + addr - 0x4000;
        assert(phys_addr <= rom_len);

        return rom[phys_addr];
    }

    return 0xFF;
}

static void mbc1_write(void *ptr, u16 addr, u8 value)
{
    Mbc1Mapper *mapper = ptr;

    if (addr >= 0x2000 && addr < 0x4000) {
        // 2000-3FFF (ROM bank number)
        mapper->rom_bank_num = (value & 0b11111) % mapper->rom_banks;
    } else if (addr < 0x6000) {
        // 4000-5FFF (upper bits of ROM bank number)
        BAIL("TODO: implement");
    } else if (addr < 0x8000) {
        // 6000-7FFF (banking mode select)
        mapper->banking_mode = value & 1;
    }
}

static Mbc1Mapper *mbc1_create(u8 rom_size_code, u8 ram_size_code)
{
    Mbc1Mapper *mapper = calloc(1, sizeof(*mapper));
    if (mapper != nullptr)
        *mapper = mbc1_init(rom_size_code, ram_size_code);

    return mapper;
}

static const MapperVTable NO_MBC_VTABLE = {
    .read = no_mbc_read,
    .write = no_mbc_write,
    .deinit = no_mbc_deinit,
};

static const MapperVTable MBC1_VTABLE = {
    .read = mbc1_read,
    .write = mbc1_write,
    .deinit = mbc1_deinit,
};

Mapper mapper_default()
{
    return (Mapper){
        .ptr = nullptr,
        .vtable = &NO_MBC_VTABLE,
    };
}

Mapper mapper_from_rom(const u8 *rom, size_t rom_len)
{
    if (rom_len < 0x8000)
        BAIL("rom_len must be at least 0x8000 bytes long");

    u8 cart_type = rom[ROM_HEADER_CART_TYPE];
    u8 rom_size_code = rom[ROM_HEADER_ROM_SIZE];
    u8 ram_size_code = rom[ROM_HEADER_RAM_SIZE];

    if (ram_size_code == 1)
        BAIL("invalid RAM size");

    switch (cart_type) {
        case CART_TYPE_ROM_ONLY:
            return (Mapper){nullptr, &NO_MBC_VTABLE};
        case CART_TYPE_MBC1:
            return (Mapper){mbc1_create(rom_size_code, ram_size_code),
                            &MBC1_VTABLE};
        default:
            BAIL("unimplemented mapper: $%02X", cart_type);
    }
}

u8 mapper_read(const Mapper *mapper, const u8 *rom, size_t rom_len, u16 addr)
{
    return mapper->vtable->read(mapper->ptr, rom, rom_len, addr);
}

void mapper_write(Mapper *mapper, u16 addr, u8 value)
{
    mapper->vtable->write(mapper->ptr, addr, value);
}

void mapper_deinit(Mapper *mapper)
{
    mapper->vtable->deinit(mapper->ptr);
    free(mapper->ptr);
    mapper->ptr = nullptr;
}

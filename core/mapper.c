#include "mapper.h"
#include "cart.h"
#include "util.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

void mapper_box_deinit(Mapper mapper[static 1])
{
    mapper->vtable->deinit(mapper->ptr);
    free(mapper->ptr);
    mapper->ptr = nullptr;
}

static void no_mbc_deinit_v([[maybe_unused]] void *ptr)
{
}

static u8 no_mbc_read_v([[maybe_unused]] void *ptr, size_t rom_len,
                        const u8 rom[static rom_len], u16 addr)
{
    if (addr < rom_len)
        return rom[addr];

    return 0xFF;
}

static void no_mbc_write_v([[maybe_unused]] void *ptr,
                           [[maybe_unused]] u16 addr, [[maybe_unused]] u8 value)
{
}

static const MapperVTable no_mbc_vtable = {
    .deinit = no_mbc_deinit_v,
    .read = no_mbc_read_v,
    .write = no_mbc_write_v,
};

static const Mapper no_mbc_mapper = {
    .ptr = nullptr,
    .vtable = &no_mbc_vtable,
};

typedef struct {
    size_t rom_banks;
    u8 rom_bank_num;
    u8 rom_bank_num_upp;
    u8 banking_mode;
} Mbc1Mapper;

static Mbc1Mapper mbc1_init(u8 rom_size, u8 ram_size)
{
    if (ram_size != 0)
        BAIL();

    return (Mbc1Mapper){
        .rom_banks = rom_banks_from_size_code(rom_size),
        .banking_mode = 0,
        .rom_bank_num = 0,
        .rom_bank_num_upp = 0,
    };
}

static void mbc1_deinit_v(void *ptr)
{
    [[maybe_unused]] Mbc1Mapper *mapper = ptr;
}

static u8 mbc1_read_v(void *ptr, size_t rom_len, const u8 rom[static rom_len],
                      u16 addr)
{
    Mbc1Mapper *mapper = ptr;

    if (mapper->banking_mode == 1)
        BAIL("read from $%04X, mode = %i", addr, mapper->banking_mode);

    if (addr < 0x4000) { // 0000-3FFF (ROM bank X0)
        assert(addr <= rom_len);
        return rom[addr];
    }

    if (addr < 0x8000) { // 4000-7FFF (ROM bank 01-7F)
        size_t bank_num = mapper->rom_bank_num;
        if (bank_num == 0)
            bank_num = 1;

        size_t bank_num_upp = (size_t)mapper->rom_bank_num_upp << 19;

        u16 phys_addr = bank_num_upp | (bank_num << 14) | (addr & 0x3FFF);
        return rom[phys_addr % rom_len];
    }

    return 0xFF;
}

static void mbc1_write_v(void *ptr, u16 addr, u8 value)
{
    Mbc1Mapper *mapper = ptr;
    if (addr >= 0x2000 && addr < 0x4000) {
        // 2000-3FFF (ROM bank number)
        mapper->rom_bank_num = (value & 0b11111) % mapper->rom_banks;
    } else if (addr < 0x6000) {
        // 4000-5FFF (upper bits of ROM bank number)
        mapper->rom_bank_num_upp = value & 0b11;
    } else if (addr < 0x8000) {
        // 6000-7FFF (banking mode select)
        mapper->banking_mode = value & 1;
    }
}

IMPL_UPCASTS(Mbc1Mapper, mbc1, Mapper, mapper, .deinit = mbc1_deinit_v,
             .read = mbc1_read_v, .write = mbc1_write_v)

Mapper mapper_default()
{
    return no_mbc_mapper;
}

Mapper mapper_from_rom(size_t rom_len, const u8 rom[static rom_len])
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
            return no_mbc_mapper;
        case CART_TYPE_MBC1:
        case CART_TYPE_MBC1_RAM:
            return mbc1_into_mapper(mbc1_init(rom_size_code, ram_size_code));
        default:
            BAIL("unimplemented mapper: $%02X", cart_type);
    }
}

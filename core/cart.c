#include "cart.h"
#include "macros.h"
#include <stddef.h>

static constexpr CartridgeType RAM_CART_TYPES[] = {
    CART_TYPE_MBC1_RAM,
    CART_TYPE_MBC1_RAM_BATTERY,
    CART_TYPE_MBC2,
    CART_TYPE_MBC2_BATTERY,
    CART_TYPE_ROM_RAM,
    CART_TYPE_ROM_RAM_BATTERY,
    CART_TYPE_MMM01_RAM,
    CART_TYPE_MMM01_RAM_BATTERY,
    CART_TYPE_MBC3_TIMER_RAM_BATTERY,
    CART_TYPE_MBC3_RAM,
    CART_TYPE_MBC3_RAM_BATTERY,
    CART_TYPE_MBC5_RAM,
    CART_TYPE_MBC5_RAM_BATTERY,
    CART_TYPE_MBC5_RUMBLE_RAM,
    CART_TYPE_MBC5_RUMBLE_RAM_BATTERY,
    CART_TYPE_MBC7_SENSOR_RUMBLE_RAM_BATTERY,
    CART_TYPE_HUC1_RAM_BATTERY,

};
static constexpr size_t RAM_CART_TYPES_LEN = ARRAY_LEN(RAM_CART_TYPES);

bool cart_type_has_ram(CartridgeType cart_type)
{
    for (size_t i = 0; i < RAM_CART_TYPES_LEN; ++i) {
        if (cart_type == RAM_CART_TYPES[i])
            return true;
    }

    return false;
}

size_t rom_banks_from_size_code(u8 rom_size_code)
{
    if (rom_size_code > 0x08)
        BAIL("invalid ROM size code: $%02X", rom_size_code);

    return 1 << (rom_size_code + 1);
}

size_t ram_banks_from_size_code(u8 ram_size_code)
{
    switch (ram_size_code) {
        case 0x00:
            return 0;
        case 0x02:
            return 1;
        case 0x03:
            return 4;
        case 0x04:
            return 16;
        case 0x05:
            return 8;
        default:
            BAIL("invalid RAM size code: $%02X", ram_size_code);
    }
}

GameInfo get_game_info(const u8 *rom)
{
    GameInfo out = {
        .title = {},
        .cart_type = rom[ROM_HEADER_CART_TYPE],
        .ram_size = rom[ROM_HEADER_RAM_SIZE],
        .rom_size = rom[ROM_HEADER_ROM_SIZE],
    };

    memcpy(out.title, (char *)&rom[ROM_HEADER_TITLE], sizeof(out.title));
    return out;
}

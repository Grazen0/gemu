#ifndef GEMU_CART_H
#define GEMU_CART_H

#include "util.h"
#include <stddef.h>

typedef enum : u16 {
    ROM_HEADER_NINTENDO_LOGO = 0x104,
    ROM_HEADER_TITLE = 0x134,
    ROM_HEADER_MANUFACTURER_CODE = 0x13F,
    ROM_HEADER_CGB_FLAG = 0x143,
    ROM_HEADER_NEW_LICENSEE_CODE = 0x144,
    ROM_HEADER_SGB_FLAG = 0x146,
    ROM_HEADER_CART_TYPE = 0x147,
    ROM_HEADER_ROM_SIZE = 0x148,
    ROM_HEADER_RAM_SIZE = 0x149,
    ROM_HEADER_DESTINATION_CODE = 0x14A,
    ROM_HEADER_OLD_LICENSEE_CODE = 0x14B,
    ROM_HEADER_MASK_ROM_VERSION_NUMBER = 0x14C,
    ROM_HEADER_CHECKSUM = 0x14D,
    ROM_HEADER_GLOBAL_CHECKSUM = 0x14E,
} RomHeader;

typedef enum : u8 {
    CART_TYPE_ROM_ONLY = 0x00,
    CART_TYPE_MBC1 = 0x01,
    CART_TYPE_MBC1_RAM = 0x02,
    CART_TYPE_MBC1_RAM_BATTERY = 0x03,
    CART_TYPE_MBC2 = 0x05,
    CART_TYPE_MBC2_BATTERY = 0x06,
    CART_TYPE_ROM_RAM = 0x08,
    CART_TYPE_ROM_RAM_BATTERY = 0x09,
    CART_TYPE_MMM01 = 0x0B,
    CART_TYPE_MMM01_RAM = 0x0C,
    CART_TYPE_MMM01_RAM_BATTERY = 0x0D,
    CART_TYPE_MBC3_TIMER_BATTERY = 0x0F,
    CART_TYPE_MBC3_TIMER_RAM_BATTERY = 0x10,
    CART_TYPE_MBC3 = 0x11,
    CART_TYPE_MBC3_RAM = 0x12,
    CART_TYPE_MBC3_RAM_BATTERY = 0x13,
    CART_TYPE_MBC5 = 0x19,
    CART_TYPE_MBC5_RAM = 0x1A,
    CART_TYPE_MBC5_RAM_BATTERY = 0x1B,
    CART_TYPE_MBC5_RUMBLE = 0x1C,
    CART_TYPE_MBC5_RUMBLE_RAM = 0x1D,
    CART_TYPE_MBC5_RUMBLE_RAM_BATTERY = 0x1E,
    CART_TYPE_MBC6 = 0x20,
    CART_TYPE_MBC7_SENSOR_RUMBLE_RAM_BATTERY = 0x22,
    CART_TYPE_POCKET_CAMERA = 0xFC,
    CART_TYPE_BANDAI_TAMA5 = 0xFD,
    CART_TYPE_HUC3 = 0xFE,
    CART_TYPE_HUC1_RAM_BATTERY = 0xFF,
} CartType;

typedef struct {
    char title[17];
    CartType cart_type;
    u8 ram_size;
    u8 rom_size;
} CartInfo;

[[nodiscard]] bool cart_type_has_ram(CartType cart_type);

[[nodiscard]] size_t rom_banks_from_size_code(u8 rom_size_code);

[[nodiscard]] size_t ram_banks_from_size_code(u8 ram_size_code);

[[nodiscard]] CartInfo cart_info_from_rom(const u8 *rom);

#endif

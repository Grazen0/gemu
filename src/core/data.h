#ifndef CORE_DATA_H
#define CORE_DATA_H

#include <stddef.h>
#include <stdint.h>

extern const size_t ROM_ENTRY_POINT;
extern const size_t ROM_NINTENDO_LOGO;
extern const size_t ROM_TITLE;
extern const size_t ROM_MANUFACTURER_CODE;
extern const size_t ROM_CGB_FLAG;
extern const size_t ROM_NEW_LICENSEE_CODE;
extern const size_t ROM_SGB_FLAG;
extern const size_t ROM_CARTRIDGE_TYPE;
extern const size_t ROM_ROM_SIZE;
extern const size_t ROM_RAM_SIZE;
extern const size_t ROM_DESTINATION_CODE;
extern const size_t ROM_OLD_LICENSEE_CODE;
extern const size_t ROM_MASK_ROM_VERSION_NUMBER;
extern const size_t ROM_HEADER_CHECKSUM;
extern const size_t ROM_GLOBAL_CHECKSUM;

enum CartridgeType {
    CTYPE_ROM_ONLY = 0x00,
    CTYPE_MCB1 = 0x01,
    CTYPE_MCB1_RAM = 0x02,
    CTYPE_MCB1_RAM_BATTERY = 0x03,
    CTYPE_MBC2 = 0x05,
    CTYPE_MBC2_BATTERY = 0x06,
    CTYPE_ROM_RAM = 0x08,
    CTYPE_ROM_RAM_BATTERY = 0x09,
    CTYPE_MMM01 = 0x0B,
    CTYPE_MMM01_RAM = 0x0C,
    CTYPE_MMM01_RAM_BATTERY = 0x0D,
    CTYPE_MBC3_TIMER_BATTERY = 0x0F,
    CTYPE_MBC3_TIMER_RAM_BATTERY = 0x10,
    CTYPE_MBC3 = 0x11,
    CTYPE_MBC3_RAM = 0x12,
    CTYPE_MBC3_RAM_BATTERY = 0x13,
    CTYPE_MBC5 = 0x19,
    CTYPE_MBC5_RAM = 0x1A,
    CTYPE_MBC5_RAM_BATTERY = 0x1B,
    CTYPE_MBC5_RUMBLE = 0x1C,
    CTYPE_MBC5_RUMBLE_RAM = 0x1D,
    CTYPE_MBC5_RUMBLE_RAM_BATTERY = 0x1E,
    CTYPE_MBC6 = 0x20,
    CTYPE_MBC7_SENSOR_RUMBLE_RAM_BATTERY = 0x22,
    CTYPE_POCKET_CAMERA = 0xFC,
    CTYPE_BANDAI_TAMA5 = 0xFD,
    CTYPE_HUC3 = 0xFE,
    CTYPE_HUC1_RAM_BATTERY = 0xFF,
};

char* get_rom_title(uint8_t* rom);

#endif

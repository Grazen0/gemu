#ifndef CORE_DATA_H
#define CORE_DATA_H

#include <stddef.h>

typedef enum RomData {
    RomData_ENTRY_POINT = 0x100,
    RomData_NINTENDO_LOGO = 0x104,
    RomData_TITLE = 0x134,
    RomData_MANUFACTURER_CODE = 0x13F,
    RomData_CGB_FLAG = 0x143,
    RomData_NEW_LICENSEE_CODE = 0x144,
    RomData_SGB_FLAG = 0x146,
    RomData_CARTRIDGE_TYPE = 0x147,
    RomData_ROM_SIZE = 0x148,
    RomData_RAM_SIZE = 0x149,
    RomData_DESTINATION_CODE = 0x14A,
    RomData_OLD_LICENSEE_CODE = 0x14B,
    RomData_MASK_ROM_VERSION_NUMBER = 0x14C,
    RomData_HEADER_CHECKSUM = 0x14D,
    RomData_GLOBAL_CHECKSUM = 0x14E,
} RomData;

enum CartridgeType {
    CartridgeType_ROM_ONLY = 0x00,
    CartridgeType_MCB1 = 0x01,
    CartridgeType_MCB1_RAM = 0x02,
    CartridgeType_MCB1_RAM_BATTERY = 0x03,
    CartridgeType_MBC2 = 0x05,
    CartridgeType_MBC2_BATTERY = 0x06,
    CartridgeType_ROM_RAM = 0x08,
    CartridgeType_ROM_RAM_BATTERY = 0x09,
    CartridgeType_MMM01 = 0x0B,
    CartridgeType_MMM01_RAM = 0x0C,
    CartridgeType_MMM01_RAM_BATTERY = 0x0D,
    CartridgeType_MBC3_TIMER_BATTERY = 0x0F,
    CartridgeType_MBC3_TIMER_RAM_BATTERY = 0x10,
    CartridgeType_MBC3 = 0x11,
    CartridgeType_MBC3_RAM = 0x12,
    CartridgeType_MBC3_RAM_BATTERY = 0x13,
    CartridgeType_MBC5 = 0x19,
    CartridgeType_MBC5_RAM = 0x1A,
    CartridgeType_MBC5_RAM_BATTERY = 0x1B,
    CartridgeType_MBC5_RUMBLE = 0x1C,
    CartridgeType_MBC5_RUMBLE_RAM = 0x1D,
    CartridgeType_MBC5_RUMBLE_RAM_BATTERY = 0x1E,
    CartridgeType_MBC6 = 0x20,
    CartridgeType_MBC7_SENSOR_RUMBLE_RAM_BATTERY = 0x22,
    CartridgeType_POCKET_CAMERA = 0xFC,
    CartridgeType_BANDAI_TAMA5 = 0xFD,
    CartridgeType_HUC3 = 0xFE,
    CartridgeType_HUC1_RAM_BATTERY = 0xFF,
};

#endif

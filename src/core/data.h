#ifndef CORE_DATA_H
#define CORE_DATA_H

#include <stddef.h>
#include <stdint.h>

typedef enum RomData : uint16_t {
    RomData_EntryPoint = 0x100,
    RomDjta_NintendoLogo = 0x104,
    RomData_Title = 0x134,
    RomData_ManufacturerCode = 0x13F,
    RomData_CgbFlag = 0x143,
    RomData_NewLicenseeCode = 0x144,
    RomData_SgbFlag = 0x146,
    RomData_CartridgeType = 0x147,
    RomData_RomSize = 0x148,
    RomData_RamSize = 0x149,
    RomData_DestinationCode = 0x14A,
    RomData_OldLicenseeCode = 0x14B,
    RomData_MaskRomVersionNumber = 0x14C,
    RomData_HeaderChecksum = 0x14D,
    RomData_GlobalChecksum = 0x14E,
} RomData;

typedef enum CartridgeType : uint8_t {
    CartridgeType_RomOnly = 0x00,
    CartridgeType_Mcb1 = 0x01,
    CartridgeType_Mcb1Ram = 0x02,
    CartridgeType_Mcb1RamBattery = 0x03,
    CartridgeType_Mbc2 = 0x05,
    CartridgeType_Mbc2Battery = 0x06,
    CartridgeType_RomRam = 0x08,
    CartridgeType_RomRamBattery = 0x09,
    CartridgeType_Mmm01 = 0x0B,
    CartridgeType_Mmm01Ram = 0x0C,
    CartridgeType_Mmm01RamBattery = 0x0D,
    CartridgeType_Mbc3TimerBattery = 0x0F,
    CartridgeType_Mbc3TimerRamBattery = 0x10,
    CartridgeType_Mbc3 = 0x11,
    CartridgeType_Mbc3Ram = 0x12,
    CartridgeType_Mbc3RamBattery = 0x13,
    CartridgeType_Mbc5 = 0x19,
    CartridgeType_Mbc5Ram = 0x1A,
    CartridgeType_Mbc5RamBattery = 0x1B,
    CartridgeType_Mbc5Rumble = 0x1C,
    CartridgeType_Mbc5RumbleRam = 0x1D,
    CartridgeType_Mbc5RumbleRamBattery = 0x1E,
    CartridgeType_Mbc6 = 0x20,
    CartridgeType_Mbc7SensorRumbleRamBattery = 0x22,
    CartridgeType_PocketCamera = 0xFC,
    CartridgeType_BandaiTama5 = 0xFD,
    CartridgeType_Huc3 = 0xFE,
    CartridgeType_Huc1RamBattery = 0xFF,
} CartridgeType;

#endif

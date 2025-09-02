#include "data.h"
#include "macros.h"

bool CartridgeType_has_ram(const CartridgeType self)
{
    return self == CartridgeType_Mbc1Ram ||
           self == CartridgeType_Mbc1RamBattery || self == CartridgeType_Mbc2 ||
           self == CartridgeType_Mbc2Battery || self == CartridgeType_RomRam ||
           self == CartridgeType_RomRamBattery ||
           self == CartridgeType_Mmm01Ram ||
           self == CartridgeType_Mmm01RamBattery ||
           self == CartridgeType_Mbc3TimerRamBattery ||
           self == CartridgeType_Mbc3Ram ||
           self == CartridgeType_Mbc3RamBattery ||
           self == CartridgeType_Mbc5Ram ||
           self == CartridgeType_Mbc5RamBattery ||
           self == CartridgeType_Mbc5RumbleRam ||
           self == CartridgeType_Mbc5RumbleRamBattery ||
           self == CartridgeType_Mbc7SensorRumbleRamBattery ||
           self == CartridgeType_Huc1RamBattery;
}

size_t rom_banks_from_size_code(const u8 rom_size_code)
{
    BAIL_IF(rom_size_code > 0x08, "invalid ROM size code: $%02X",
            rom_size_code);

    return 1 << (rom_size_code + 1);
}

size_t ram_banks_from_size_code(const u8 ram_size_code)
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

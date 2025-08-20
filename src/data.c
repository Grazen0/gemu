#include "data.h"

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

#include "data.h"
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
static constexpr size_t RAM_CART_TYPES_LEN =
    sizeof(RAM_CART_TYPES) / sizeof(RAM_CART_TYPES[0]);

bool CartridgeType_has_ram(CartridgeType cart_type)
{
    for (size_t i = 0; i < RAM_CART_TYPES_LEN; ++i) {
        if (cart_type == RAM_CART_TYPES[i])
            return true;
    }

    return false;
}

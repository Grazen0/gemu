#include "num.h"
#include "stdinc.h"

u16 concat_u16(const u8 hi, const u8 lo)
{
    return ((u16)hi << 8) | (u16)lo;
}

void set_bits(u8 *const dest, const u8 mask, const bool value)
{
    if (value) {
        *dest |= mask;
    } else {
        *dest &= ~mask;
    }
}

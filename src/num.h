#ifndef GEMU_NUM_H
#define GEMU_NUM_H

#include "stdinc.h"

[[nodiscard]] inline u16 concat_u16(const u8 hi, const u8 lo)
{
    return ((u16)hi << 8) | (u16)lo;
}

inline void set_bits(u8 *const dest, const u8 mask, const bool value)
{
    if (value)
        *dest |= mask;
    else
        *dest &= ~mask;
}

#endif

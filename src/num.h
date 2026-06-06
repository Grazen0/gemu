#ifndef GEMU_NUM_H
#define GEMU_NUM_H

#include "stdinc.h"

[[nodiscard]] static inline u16 concat_u16(u8 hi, u8 lo)
{
    return ((u16)hi << 8) | (u16)lo;
}

static inline void set_bits(u8 *dest, u8 mask, bool value)
{
    if (value)
        *dest |= mask;
    else
        *dest &= ~mask;
}

#endif

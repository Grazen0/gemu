#ifndef GEMU_UTIL_H
#define GEMU_UTIL_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

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

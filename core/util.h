#ifndef GEMU_UTIL_H
#define GEMU_UTIL_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#define BAIL(...)                                                           \
    do {                                                                    \
        __VA_OPT__(fprintf(stderr, "panic (%s:%d): ", __FILE__, __LINE__);) \
        __VA_OPT__(if (0))                                                  \
        fprintf(stderr, "panic (%s:%d)", __FILE__, __LINE__);               \
        __VA_OPT__(fprintf(stderr, __VA_ARGS__);)                           \
        fputc('\n', stderr);                                                \
        abort();                                                            \
    } while (0)

#define DECL_UPCASTS(Derived, derived, Base, base) \
    Base derived##_as_##base(Derived *derived);    \
    Base derived##_into_##base(Derived base);

#define IMPL_UPCASTS(Derived, derived, Base, base, ...)         \
    static const Base##VTable derived##_vtable = {__VA_ARGS__}; \
    Base derived##_as_##base(Derived *derived)                  \
    {                                                           \
        return (Base){                                          \
            .ptr = derived,                                     \
            .vtable = &derived##_vtable,                        \
        };                                                      \
    }                                                           \
    Base derived##_into_##base(Derived base)                    \
    {                                                           \
        Derived *ptr = calloc(1, sizeof(*ptr));                 \
        assert(ptr != nullptr);                                 \
        memcpy(ptr, &base, sizeof(*ptr));                       \
        return (Base){                                          \
            .ptr = ptr,                                         \
            .vtable = &derived##_vtable,                        \
        };                                                      \
    }

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

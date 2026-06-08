#ifndef GEMU_MACROS_H
#define GEMU_MACROS_H

#include <assert.h>
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

#define DECLARE_UPCASTS(Derived, derived, Base, base) \
    Base derived##_as_##base(Derived *derived);       \
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

#endif

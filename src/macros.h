#ifndef GEMU_MACROS_H
#define GEMU_MACROS_H

#include <stdio.h>
#include <stdlib.h>

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#define BAIL(...)                                                           \
    do {                                                                    \
                                                                            \
        __VA_OPT__(fprintf(stderr, "panic (%s:%d): ", __FILE__, __LINE__);) \
        __VA_OPT__(if (0))                                                  \
        fprintf(stderr, "panic (%s:%d)", __FILE__, __LINE__);               \
        __VA_OPT__(fprintf(stderr, __VA_ARGS__);)                           \
        fputc('\n', stderr);                                                \
        abort();                                                            \
    } while (0)

#endif

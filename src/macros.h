#ifndef GEMU_MACROS_H
#define GEMU_MACROS_H

#include "log.h"
#include <stdio.h>
#include <stdlib.h>

#define BAIL(...)                                      \
    do {                                               \
        log_error("BAIL (%s:%d)", __FILE__, __LINE__); \
        __VA_OPT__(log_error(__VA_ARGS__);)            \
        exit(1);                                       \
    } while (0)

#define BAIL_IF(cond, ...)                                                 \
    do {                                                                   \
        if (cond) {                                                        \
            log_error("BAIL_IF('%s') (%s:%d)", #cond, __FILE__, __LINE__); \
            __VA_OPT__(log_error(__VA_ARGS__);)                            \
            exit(1);                                                       \
        }                                                                  \
    } while (0)

#define BAIL_IF_NULL(ptr, ...)                                                 \
    do {                                                                       \
        if ((ptr) == nullptr) {                                                \
            log_error("BAIL_IF_NULL('%s') (%s:%d)", #ptr, __FILE__, __LINE__); \
            __VA_OPT__(log_error(__VA_ARGS__);)                                \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#endif

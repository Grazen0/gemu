#ifndef COMMON_CONTROL_H
#define COMMON_CONTROL_H

#include <stdio.h> 
#include <stdlib.h>

#define BAIL(...)                                              \
    do {                                                       \
        fprintf(stderr, "BAIL (%s:%d): ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                          \
        fputc('\n', stderr);                                   \
        exit(1);                                               \
    } while (0);

#define BAIL_IF(cond, ...)                                                         \
    do {                                                                           \
        if (cond) {                                                                \
            fprintf(stderr, "BAIL_IF('%s') (%s:%d): ", #cond, __FILE__, __LINE__); \
            fprintf(stderr, __VA_ARGS__);                                          \
            fputc('\n', stderr);                                                   \
            exit(1);                                                               \
        }                                                                          \
    } while (0);

#endif

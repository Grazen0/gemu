#ifndef LIB_LOG_H
#define LIB_LOG_H

#include <stdlib.h>
#include "SDL3/SDL_log.h"

#define BAIL(...)                                          \
    {                                                      \
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, __VA_ARGS__); \
        exit(1);                                           \
    }

#define BAIL_IF(cond, ...)                                 \
    if (cond) {                                            \
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, __VA_ARGS__); \
        exit(1);                                           \
    }

#endif

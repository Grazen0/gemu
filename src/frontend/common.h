#ifndef GEMU_FRONTEND_COMMON_H
#define GEMU_FRONTEND_COMMON_H

#include "game_boy.h"
#include "macros.h"
#include "stdinc.h"
#include <stddef.h>

static constexpr u8 PALETTE_RGB[][3] = {
    {186, 218, 85},
    {130, 153, 59},
    { 74,  87, 34},
    { 19,  22,  8}
};

static constexpr size_t PALETTE_RGB_LEN = ARRAY_LEN(PALETTE_RGB);

typedef struct {
    const char *name;
    int (*run)(GameBoy *gb);
} Frontend;

extern const Frontend selected_frontend;

#endif

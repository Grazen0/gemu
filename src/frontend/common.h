#ifndef GEMU_FRONTEND_COMMON_H
#define GEMU_FRONTEND_COMMON_H

#include "game_boy.h"
#include "macros.h"
#include "scheduler.h"
#include "stdinc.h"
#include <stddef.h>

static constexpr int WINDOW_INIT_WIDTH = GB_LCD_WIDTH * 4;
static constexpr int WINDOW_INIT_HEIGHT = GB_LCD_HEIGHT * 4;
static constexpr float GB_LCD_ASPECT_RATIO =
    (float)GB_LCD_WIDTH / GB_LCD_HEIGHT;

static constexpr u8 PALETTE_RGB[][3] = {
    {186, 218, 85},
    {130, 153, 59},
    { 74,  87, 34},
    { 19,  22,  8}
};

static constexpr size_t PALETTE_RGB_LEN = ARRAY_LEN(PALETTE_RGB);

typedef struct {
    float x;
    float y;
    float w;
    float h;
} FitRect;

FitRect fit_rect_to_ratio(float cx, float cy, float cw, float ch, float ratio);

typedef struct {
    const char *name;
    int (*run)(GameBoy *, Scheduler *);
} Frontend;

extern const Frontend selected_frontend;

#endif

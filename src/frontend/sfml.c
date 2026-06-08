#include "frontend/sfml.h"
#include "common.h"
#include "game_boy.h"
#include "scheduler.h"
#include "stdinc.h"
#include <CSFML/Graphics.h>
#include <CSFML/Window.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// even though only R,G,B values get modified, sfml wants RGBA.
#define RGBA_ALLOC_SIZE 4
#define TO_COLOR(rgb) {(rgb)[0], (rgb)[1], (rgb)[2], 255}

static void update_pixels(const GameBoy *gb, sfTexture *tex, u8 *rgb_buf)
{
    static const sfColor PALETTE[] = {
        TO_COLOR(PALETTE_RGB[0]), TO_COLOR(PALETTE_RGB[1]),
        TO_COLOR(PALETTE_RGB[2]), TO_COLOR(PALETTE_RGB[3])};
    static_assert(ARRAY_LEN(PALETTE) == PALETTE_RGB_LEN);

    for (size_t p = 0; p < (size_t)GB_LCD_WIDTH * GB_LCD_HEIGHT; ++p) {
        const size_t y = p / GB_LCD_WIDTH;
        const size_t x = p % GB_LCD_WIDTH;

        u8 color = gb->scanout_buf[y][x];
        const sfColor out_col = PALETTE[color];

        const size_t dst = p * 4;
        rgb_buf[dst] = out_col.r;
        rgb_buf[dst + 1] = out_col.g;
        rgb_buf[dst + 2] = out_col.b;
        rgb_buf[dst + 3] = out_col.a;
    }

    sfTexture_updateFromPixels(tex, rgb_buf,
                               (sfVector2u){GB_LCD_WIDTH, GB_LCD_HEIGHT},
                               (sfVector2u){0, 0});
}

static void update_screen(sfSprite *spr, sfRenderWindow *wnd)
{
    sfVector2u size = sfRenderWindow_getSize(wnd);

    float wnd_w = (float)size.x;
    float wnd_h = (float)size.y;

    float cont_ratio = wnd_w / wnd_h;
    float dest_w = wnd_w;
    float dest_h = wnd_h;
    float dest_x = 0.F;
    float dest_y = 0.F;

    if (cont_ratio > GB_LCD_ASPECT_RATIO) {
        dest_w = wnd_h * GB_LCD_ASPECT_RATIO;
        dest_x = (wnd_w - dest_w) / 2.0F;
    } else if (cont_ratio < GB_LCD_ASPECT_RATIO) {
        dest_h = wnd_w / GB_LCD_ASPECT_RATIO;
        dest_y = (wnd_h - dest_h) / 2.0F;
    }

    float scl_x = dest_w / (float)GB_LCD_WIDTH;
    float scl_y = dest_h / (float)GB_LCD_HEIGHT;

    sfSprite_setScale(spr, (sfVector2f){scl_x, scl_y});
    sfSprite_setPosition(spr, (sfVector2f){dest_x, dest_y});

    sfRenderWindow_clear(wnd, sfBlack);
    sfRenderWindow_drawSprite(wnd, spr, nullptr);
    sfRenderWindow_display(wnd);
}

static bool *map_joypad_btn(JoypadButtons *joypad, sfKeyCode key)
{
    switch (key) {
        case sfKeyEnter:
            return &joypad->start;
        case sfKeySpace:
            return &joypad->select;
        case sfKeyUp:
            return &joypad->up;
        case sfKeyDown:
            return &joypad->down;
        case sfKeyRight:
            return &joypad->right;
        case sfKeyLeft:
            return &joypad->left;
        case sfKeyX:
            return &joypad->a;
        case sfKeyZ:
            return &joypad->b;
        default:
            return nullptr;
    }
}

static int run(GameBoy *gb)
{
    sfVideoMode vid_mode = {
        {WINDOW_INIT_WIDTH, WINDOW_INIT_HEIGHT},
        32
    };
    sfRenderWindow *window = sfRenderWindow_create(
        vid_mode, "gemu", sfTitlebar | sfClose | sfResize, sfWindowed, nullptr);
    sfRenderWindow_setFramerateLimit(window, 60);

    sfVector2u tex_size = {GB_LCD_WIDTH, GB_LCD_HEIGHT};
    sfTexture *texture = sfTexture_create(tex_size);

    sfSprite *gb_spr = sfSprite_create(texture);
    u8 pixel_buffer[(size_t)GB_LCD_WIDTH * GB_LCD_HEIGHT * sizeof(u8) *
                    RGBA_ALLOC_SIZE];

    sfClock *clock = sfClock_create();
    Scheduler sched = sched_init();

    sfEvent cur_even;
    bool exiting = false;
    while (sfRenderWindow_isOpen(window) && !exiting) {

        while (sfRenderWindow_pollEvent(window, &cur_even)) {
            if (cur_even.type == sfEvtClosed) {
                sfRenderWindow_close(window);
                exiting = true;
            } else if (cur_even.type == sfEvtKeyPressed ||
                       cur_even.type == sfEvtKeyReleased) {
                bool *joypad_btn = map_joypad_btn(&gb->btns, cur_even.key.code);

                if (joypad_btn != nullptr) {
                    *joypad_btn = cur_even.type == sfEvtKeyPressed;
                    break;
                }
            }
        }

        if (exiting)
            break;

        sfTime elapsed = sfClock_getElapsedTime(clock);
        long double cur_time =
            (long double)sfTime_asMicroseconds(elapsed) / 1000000.0L;
        u64 cur_time_clk = (u64)(cur_time * GB_CLK_FREQ_HZ);

        sched_dispatch_until(&sched, gb, cur_time_clk);
        update_pixels(gb, texture, pixel_buffer);
        update_screen(gb_spr, window);
    }

    sfRenderWindow_destroy(window);
    return EXIT_SUCCESS;
}

const Frontend selected_frontend = {
    .name = "sfml",
    .run = run,
};
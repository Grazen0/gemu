#include "frontend/sfml.h"
#include "common.h"
#include "game_boy.h"
#include "log.h"
#include "scheduler.h"
#include "util.h"
#include <CSFML/Graphics.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// even though only R,G,B values get modified, sfml wants RGBA.
#define RGBA_ALLOC_SIZE 4
#define TO_COLOR(rgb) {(rgb)[0], (rgb)[1], (rgb)[2], 255}

static void update_pixels(const GameBoy *gb, sfColor pixel_buf[])
{
    static const sfColor PALETTE[] = {
        TO_COLOR(PALETTE_RGB[0]),
        TO_COLOR(PALETTE_RGB[1]),
        TO_COLOR(PALETTE_RGB[2]),
        TO_COLOR(PALETTE_RGB[3]),
    };
    static_assert(ARRAY_LEN(PALETTE) == PALETTE_RGB_LEN);

    for (size_t p = 0; p < (size_t)GB_LCD_WIDTH * GB_LCD_HEIGHT; ++p) {
        size_t y = p / GB_LCD_WIDTH;
        size_t x = p % GB_LCD_WIDTH;

        u8 color = gb->scanout_buf[y][x];
        pixel_buf[p] = PALETTE[color];
    }
}

static void update_screen(sfSprite *spr, sfRenderWindow *wnd, sfView *view)
{
    sfVector2u size = sfRenderWindow_getSize(wnd);

    float wnd_w = (float)size.x;
    float wnd_h = (float)size.y;

    sfView_setSize(view, (sfVector2f){wnd_w, wnd_h});
    sfView_setCenter(view, (sfVector2f){wnd_w / 2.0F, wnd_h / 2.0F});
    sfRenderWindow_setView(wnd, view);

    FitRect fit = fit_rect_to_ratio(0, 0, wnd_w, wnd_h, GB_LCD_ASPECT_RATIO);
    float scl_x = fit.w / (float)GB_LCD_WIDTH;
    float scl_y = fit.h / (float)GB_LCD_HEIGHT;

    sfSprite_setScale(spr, (sfVector2f){scl_x, scl_y});
    sfSprite_setPosition(spr, (sfVector2f){fit.x, fit.y});

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

static void handle_event(sfEvent *event, GameBoy *gb, sfRenderWindow *window,
                         bool *quit)
{

    if (event->type == sfEvtClosed) {
        sfRenderWindow_close(window);
        *quit = true;
        return;
    }

    if (event->type == sfEvtKeyPressed || event->type == sfEvtKeyReleased) {
        bool *joypad_btn = map_joypad_btn(&gb->btns, event->key.code);

        if (joypad_btn != nullptr)
            *joypad_btn = event->type == sfEvtKeyPressed;
    }
}

static int run(GameBoy *gb, Scheduler *sched)
{
    sfVideoMode vid_mode = {
        {WINDOW_INIT_WIDTH, WINDOW_INIT_HEIGHT},
        32,
    };

    log_info("Creating window");
    sfRenderWindow *window = sfRenderWindow_create(
        vid_mode, "gemu", sfDefaultStyle, sfWindowed, nullptr);
    if (window == nullptr)
        BAIL("Could not create window.");

    sfRenderWindow_setFramerateLimit(window, 60);

    log_info("Creating view");
    sfView *view = sfView_create();
    if (view == nullptr)
        BAIL("Could not create view.");

    log_info("Creating texture");
    sfVector2u texture_size = {GB_LCD_WIDTH, GB_LCD_HEIGHT};
    sfTexture *texture = sfTexture_create(texture_size);
    if (texture == nullptr)
        BAIL("Could not create texture.");

    sfSprite *gb_spr = sfSprite_create(texture);
    if (gb_spr == nullptr)
        BAIL("Could not create texture.");

    sfColor *pixel_buf =
        calloc((size_t)GB_LCD_WIDTH * GB_LCD_HEIGHT, sizeof(*pixel_buf));
    assert(pixel_buf != nullptr);

    bool quit = false;

    sfClock *clock = sfClock_create();
    assert(clock != nullptr);

    while (sfRenderWindow_isOpen(window) && !quit) {
        sfEvent event = {};
        while (sfRenderWindow_pollEvent(window, &event))
            handle_event(&event, gb, window, &quit);

        long double cur_time = sfTime_asSeconds(sfClock_getElapsedTime(clock));
        u64 cur_time_clk = (u64)(cur_time * GB_CLK_FREQ_HZ);

        sched_dispatch_until(sched, gb, cur_time_clk);
        update_pixels(gb, pixel_buf);
        sfTexture_updateFromPixels(texture, (u8 *)pixel_buf, texture_size,
                                   (sfVector2u){0, 0});
        update_screen(gb_spr, window, view);
    }

    log_info("Cleaning up SFML");
    sfClock_destroy(clock);
    free(pixel_buf);
    sfSprite_destroy(gb_spr);
    sfTexture_destroy(texture);
    sfView_destroy(view);
    sfRenderWindow_destroy(window);
    return EXIT_SUCCESS;
}

const Frontend selected_frontend = {
    .name = "sfml",
    .run = run,
};

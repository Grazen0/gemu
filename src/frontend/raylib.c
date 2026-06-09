#include "frontend/raylib.h"
#include "frontend/common.h"
#include "game_boy.h"
#include "scheduler.h"
#include "util.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

#define TO_COLOR(rgb) {(rgb)[0], (rgb)[1], (rgb)[2], 255}

static void update_pixels(const GameBoy *gb, Texture texture, Color *pixels)
{
    static const Color PALETTE[] = {
        TO_COLOR(PALETTE_RGB[0]),
        TO_COLOR(PALETTE_RGB[1]),
        TO_COLOR(PALETTE_RGB[2]),
        TO_COLOR(PALETTE_RGB[3]),
    };
    static_assert(ARRAY_LEN(PALETTE) == PALETTE_RGB_LEN);

    for (size_t y = 0; y < GB_LCD_HEIGHT; ++y) {
        for (size_t x = 0; x < GB_LCD_WIDTH; ++x) {
            u8 color = gb->scanout_buf[y][x];
            pixels[(y * texture.width) + x] = PALETTE[color];
        }
    }
}

static void draw(GameBoy *gb, Texture texture, Color *pixels)
{
    update_pixels(gb, texture, pixels);
    UpdateTexture(texture, pixels);

    FitRect fit =
        fit_rect_to_ratio(0, 0, (float)GetScreenWidth(),
                          (float)GetScreenHeight(), GB_LCD_ASPECT_RATIO);
    Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
    Rectangle dest = {fit.x, fit.y, fit.w, fit.h};

    BeginDrawing();

    ClearBackground(BLACK);
    DrawTexturePro(texture, source, dest, Vector2Zero(), 0, WHITE);

    EndDrawing();
}

static int run(GameBoy *gb, Scheduler *sched)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_INIT_WIDTH, WINDOW_INIT_HEIGHT, "gemu");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    Color *pixels =
        MemAlloc((size_t)GB_LCD_WIDTH * GB_LCD_HEIGHT * sizeof(*pixels));
    assert(pixels != nullptr);

    Image image = {
        .data = pixels,
        .width = GB_LCD_WIDTH,
        .height = GB_LCD_HEIGHT,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };

    Texture2D texture = LoadTextureFromImage(image);

    double start = GetTime();

    while (!WindowShouldClose()) {
        long double frame_start = GetTime();

        gb->btns.start = IsKeyDown(KEY_ENTER);
        gb->btns.select = IsKeyDown(KEY_SPACE);
        gb->btns.up = IsKeyDown(KEY_UP);
        gb->btns.down = IsKeyDown(KEY_DOWN);
        gb->btns.left = IsKeyDown(KEY_LEFT);
        gb->btns.right = IsKeyDown(KEY_RIGHT);
        gb->btns.a = IsKeyDown(KEY_X);
        gb->btns.b = IsKeyDown(KEY_Z);

        long double cur_time = frame_start - start;
        u64 cur_time_clk = (u64)(cur_time * GB_CLK_FREQ_HZ);

        sched_dispatch_until(sched, gb, cur_time_clk);
        draw(gb, texture, pixels);
    }

    UnloadTexture(texture);
    UnloadImage(image);
    CloseWindow();

    return EXIT_SUCCESS;
}

const Frontend selected_frontend = {
    .name = "raylib",
    .run = run,
};

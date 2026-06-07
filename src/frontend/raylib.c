#include "frontend/raylib.h"
#include "frontend/common.h"
#include "game_boy.h"
#include "scheduler.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

static Rectangle fit_rect_to_aspect_ratio(Rectangle container,
                                          float aspect_ratio)
{
    double container_aspect_ratio = container.width / container.height;

    if (container_aspect_ratio > aspect_ratio) {
        // Stretched out horizontally
        float w = container.height * aspect_ratio;
        return (Rectangle){
            .x = container.x + (container.width / 2.0F) - (w / 2.0F),
            .y = container.y,
            .width = w,
            .height = container.height,
        };
    }

    if (container_aspect_ratio < aspect_ratio) {
        // Stretched out vertically
        float h = container.width / aspect_ratio;
        return (Rectangle){
            .x = container.x,
            .y = container.y + (container.height / 2.0F) - (h / 2.0F),
            .width = container.width,
            .height = h,
        };
    }

    // Exactly the right aspect ratio
    return container;
}

static void update_pixels(const GameBoy *gb, Texture texture, Color *pixels,
                          Color palette[])
{
    for (size_t y = 0; y < GB_LCD_HEIGHT; ++y) {
        for (size_t x = 0; x < GB_LCD_WIDTH; ++x) {
            u8 color = gb->scanout_buf[y][x];
            pixels[(y * texture.width) + x] = palette[color];
        }
    }
}

static void draw(GameBoy *gb, Texture texture, Color *pixels, Color palette[])
{
    update_pixels(gb, texture, pixels, palette);
    UpdateTexture(texture, pixels);

    int width = GetScreenWidth();
    int height = GetScreenHeight();

    Rectangle source = {0, 0, (float)texture.width, (float)texture.height};
    Rectangle win_rect = {0, 0, (float)width, (float)height};
    Rectangle dest = fit_rect_to_aspect_ratio(win_rect, GB_LCD_ASPECT_RATIO);

    BeginDrawing();

    ClearBackground(BLACK);
    DrawTexturePro(texture, source, dest, Vector2Zero(), 0, WHITE);

    EndDrawing();
}

static void build_color_palette(Color out_palette[])
{
    for (size_t i = 0; i < PALETTE_RGB_LEN; ++i) {
        const u8 *rgb = PALETTE_RGB[i];
        out_palette[i] = (Color){rgb[0], rgb[1], rgb[2], 255};
    }
}

static int run(GameBoy *gb)
{
    Color palette[PALETTE_RGB_LEN] = {};
    build_color_palette(palette);

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
    Scheduler sched = sched_init();

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

        while (sched_cur_time(&sched) < cur_time_clk)
            sched_dispatch(&sched, gb);

        draw(gb, texture, pixels, palette);
    }

    sched_deinit(&sched);
    UnloadTexture(texture);
    UnloadImage(image);
    CloseWindow();

    return EXIT_SUCCESS;
}

const Frontend selected_frontend = {
    .name = "raylib",
    .run = run,
};

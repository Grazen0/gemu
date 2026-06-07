#include "frontend.h"
#include "game_boy.h"
#include "scheduler.h"
#include "stdinc.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static constexpr int TARGET_FPS = 60;
static constexpr double TARGET_DELTA = 1.0 / TARGET_FPS;

static constexpr u8 PALETTE_RGB[][3] = {
    {186, 218, 85},
    {130, 153, 59},
    { 74,  87, 34},
    { 19,  22,  8}
};

static double sdl_get_performance_time()
{
    return (double)SDL_GetPerformanceCounter() /
           (double)SDL_GetPerformanceFrequency();
}

static SDL_FRect fit_rect_to_aspect_ratio(const SDL_FRect *container,
                                          float aspect_ratio)
{
    double container_aspect_ratio = container->w / container->h;

    if (container_aspect_ratio > aspect_ratio) {
        // Stretched out horizontally
        float w = container->h * aspect_ratio;
        return (SDL_FRect){
            .w = w,
            .h = container->h,
            .x = container->x + (container->w / 2.0F) - (w / 2.0F),
            .y = container->y,
        };
    }

    if (container_aspect_ratio < aspect_ratio) {
        // Stretched out vertically
        float h = container->w / aspect_ratio;
        return (SDL_FRect){
            .w = container->w,
            .h = h,
            .x = container->x,
            .y = container->y + (container->h / 2.0F) - (h / 2.0F),
        };
    }

    // Exactly the right aspect ratio
    return *container;
}

static constexpr size_t PALETTE_RGB_LEN =
    sizeof(PALETTE_RGB) / sizeof(PALETTE_RGB[0]);

static bool *map_joypad_btn(JoypadButtons *joypad, SDL_Keycode key,
                            SDL_Keymod mod)
{
    if (mod != SDL_KMOD_NONE)
        return nullptr;

    switch (key) {
        case SDLK_RETURN:
            return &joypad->start;
        case SDLK_SPACE:
            return &joypad->select;
        case SDLK_UP:
            return &joypad->up;
        case SDLK_DOWN:
            return &joypad->down;
        case SDLK_RIGHT:
            return &joypad->right;
        case SDLK_LEFT:
            return &joypad->left;
        case SDLK_X:
            return &joypad->a;
        case SDLK_Z:
            return &joypad->b;
        default:
            return nullptr;
    }
}

static inline SDL_Keymod mask_relevant_mod(SDL_Keymod mod)
{
    return mod &
           (SDL_KMOD_CTRL | SDL_KMOD_SHIFT | SDL_KMOD_ALT | SDL_KMOD_CAPS);
}

static void handle_event(State *state, const SDL_Event *event)
{
    switch (event->type) {
        case SDL_EVENT_QUIT:
            state->quit = true;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            state->window_width = event->window.data1;
            state->window_height = event->window.data2;
            break;
        case SDL_EVENT_KEY_DOWN: {
            SDL_Keymod relevant_mod = mask_relevant_mod(event->key.mod);
            bool *joypad_btn =
                map_joypad_btn(&state->gb.btns, event->key.key, relevant_mod);

            if (joypad_btn != nullptr) {
                *joypad_btn = true;
                break;
            }
            break;
        }
        case SDL_EVENT_KEY_UP: {
            SDL_Keymod relevant_mod = mask_relevant_mod(event->key.mod);
            bool *joypad_btn =
                map_joypad_btn(&state->gb.btns, event->key.key, relevant_mod);

            if (joypad_btn != nullptr)
                *joypad_btn = false;

            break;
        }
        default:
    }
}

static void state_update_texture(const GameBoy *gb, SDL_Texture *texture,
                                 const u32 palette[])
{

    SDL_Surface *surface = nullptr;
    assert(SDL_LockTextureToSurface(texture, nullptr, &surface));

    auto pixel_format = SDL_GetPixelFormatDetails(surface->format);
    assert(pixel_format != nullptr);

    SDL_FillSurfaceRect(surface, nullptr,
                        SDL_MapRGB(pixel_format, nullptr, 0, 0, 0));

    u32 *pixels = surface->pixels;

    for (size_t y = 0; y < GB_LCD_HEIGHT; ++y) {
        for (size_t x = 0; x < GB_LCD_WIDTH; ++x) {
            u8 color = gb->scanout_buf[y][x];
            pixels[(y * surface->w) + x] = palette[color];
        }
    }

    SDL_UnlockTexture(texture);
    surface = nullptr;
}

static void render(const State *state, SDL_Renderer *renderer,
                   SDL_Texture *texture, const u32 palette[])
{
    static constexpr float ASPECT_RATIO = (float)GB_LCD_WIDTH / GB_LCD_HEIGHT;

    SDL_FRect win_rect = {
        0,
        0,
        (float)state->window_width,
        (float)state->window_height,
    };

    SDL_FRect dest_rect = fit_rect_to_aspect_ratio(&win_rect, ASPECT_RATIO);

    state_update_texture(&state->gb, texture, palette);
    SDL_RenderTexture(renderer, texture, nullptr, &dest_rect);
    SDL_RenderPresent(renderer);
}

State state_init(const u8 *boot_rom, SDL_Window *window)
{
    int window_width = 0;
    int window_height = 0;
    SDL_GetWindowSize(window, &window_width, &window_height);

    return (State){
        .gb = gb_init(boot_rom),
        .window_width = window_width,
        .window_height = window_height,
        .quit = false,
    };
}

void state_deinit(State *state)
{
    if (state == nullptr)
        return;

    gb_deinit(&state->gb);
}

void run_until_quit(State *state, SDL_Renderer *renderer)
{
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             GB_LCD_WIDTH, GB_LCD_HEIGHT);
    assert(texture != nullptr);

    u32 palette[PALETTE_RGB_LEN] = {};
    auto pixel_format = SDL_GetPixelFormatDetails(texture->format);

    for (size_t i = 0; i < PALETTE_RGB_LEN; ++i) {
        const u8 *rgb = PALETTE_RGB[i];
        palette[i] = SDL_MapRGB(pixel_format, nullptr, rgb[0], rgb[1], rgb[2]);
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    double start = sdl_get_performance_time();
    Scheduler sched = sched_init();

    while (!state->quit) {
        double frame_start = sdl_get_performance_time();

        SDL_Event event = {};
        while (SDL_PollEvent(&event))
            handle_event(state, &event);

        double cur_time = frame_start - start;
        u64 cur_time_clk = (u64)(cur_time * GB_CLK_FREQ_HZ);

        while (sched_cur_time(&sched) < cur_time_clk)
            sched_dispatch(&sched, &state->gb);

        render(state, renderer, texture, palette);

        double frame_duration = sdl_get_performance_time() - frame_start;
        double delay = TARGET_DELTA - frame_duration;

        if (delay > 0)
            SDL_DelayNS((u64)(delay * 1e9));
    }

    sched_deinit(&sched);

    SDL_DestroyTexture(texture);
    texture = nullptr;
}

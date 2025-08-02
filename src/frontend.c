#include "frontend.h"
#include "SDL3/SDL_iostream.h"
#include "control.h"
#include "cpu.h"
#include "game_boy.h"
#include "log.h"
#include "sdl.h"
#include "stdinc.h"
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_timer.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * FPS at which the application runs
 */
static constexpr int FPS = 60;

/**
 * Time-per-frame at which the application runs
 */
static constexpr double DELTA = 1.0 / FPS;

/**
 * Value of the hard limit on the game loop time accumulator
 */
static constexpr double MAX_TIME_ACCUMULATOR = 4 * DELTA;

/**
 * Frequency of DIV increments
 */
static constexpr int DIV_FREQUENCY_HZ = 16384; // 16779 Hz on SGB

/**
 * Length of PALETTE_RGB_LEN
 */
static constexpr size_t PALETTE_RGB_LEN = 4;

/**
 * Color palette to display on-screen
 */
static const u8 PALETTE_RGB[PALETTE_RGB_LEN][3] = {
    {186, 218, 85},
    {130, 153, 59},
    { 74,  87, 34},
    { 19,  22,  8}
};

/**
 * \brief Maps an index in the range 0-3 (inclusive) to its corresponding RGB
 * color.
 *
 * Will bail if color_index > 3.
 *
 * \param color_index the index of the desired color (must be in the range 0-3
 * inclusive).
 * \param pixel_format the desired pixel format.
 * \return the RGB color that color_index corresponds to, in the format pointed
 * to by pixel_format.
 *
 * \sa SDL_MapRGB
 */
static u32 map_color_index(const size_t color_index,
                           const SDL_PixelFormatDetails *const pixel_format)
{
    if (color_index >= PALETTE_RGB_LEN)
        BAIL("color index out of bounds: %zu", color_index);

    const u8 *color_rgb = PALETTE_RGB[color_index];
    return SDL_MapRGB(pixel_format, nullptr, color_rgb[0], color_rgb[1],
                      color_rgb[2]);
}

static bool *get_joypad_key(GameBoy *const gb, const SDL_Keycode key)
{
    switch (key) {
    case SDLK_RETURN:
        return &gb->joypad.start;
    case SDLK_SPACE:
        return &gb->joypad.select;
    case SDLK_UP:
        return &gb->joypad.up;
    case SDLK_DOWN:
        return &gb->joypad.down;
    case SDLK_RIGHT:
        return &gb->joypad.right;
    case SDLK_LEFT:
        return &gb->joypad.left;
    case SDLK_X:
        return &gb->joypad.a;
    case SDLK_Z:
        return &gb->joypad.b;
    default:
        return nullptr;
    }
}

static void file_callback(void *const data, const char *const *const files,
                          [[maybe_unused]] const int filter)
{
    if (files == nullptr) {
        log_error("Error selecting ROM file: %s", SDL_GetError());
        return;
    }

    if (files[0] == nullptr)
        return;

    GameBoy *const gb = data;
    const char *const rom_file = files[0];

    log_info("Loading ROM at %s", rom_file);

    size_t rom_len = 0;
    u8 *const rom = SDL_LoadFile(rom_file, &rom_len);

    if (rom == nullptr) {
        log_error("Could not load ROM file: %s", SDL_GetError());
        return;
    }

    GameBoy_load_rom(gb, rom, rom_len);
    GameBoy_log_cartridge_info(gb);
}

static void handle_event(State *const state, const SDL_Event *const event)
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
        bool *const state_key = get_joypad_key(&state->gb, event->key.key);
        if (state_key != nullptr) {
            break;
            *state_key = true;
        }

        if (event->key.mod & SDL_KMOD_CTRL && event->key.key == SDLK_O) {
            SDL_ShowOpenFileDialog(file_callback, &state->gb, nullptr, nullptr,
                                   0, nullptr, false);
        }
        break;
    }
    case SDL_EVENT_KEY_UP: {
        bool *const state_key = get_joypad_key(&state->gb, event->key.key);
        if (state_key != nullptr)
            *state_key = false;

        break;
    }
    default:
    }
}

static void update(State *const state, const double delta)
{
    Memory memory = (Memory){
        .ctx = &state->gb,
        .read = GameBoy_read_mem,
        .write = GameBoy_write_mem,
    };

    state->gb.cpu.cycle_count = 0;

    const double total_frame_cycles = GB_CPU_FREQUENCY_HZ * delta;

    const double VFRAME_DURATION = 1.0 / GB_VBLANK_FREQ;
    const int DIV_FREQUENCY_CYCLES = GB_CPU_FREQUENCY_HZ / DIV_FREQUENCY_HZ;

    while (state->cycle_accumulator < total_frame_cycles) {
        const double actual_vframe_time =
            state->vframe_time +
            (state->cycle_accumulator / GB_CPU_FREQUENCY_HZ);
        double progress = actual_vframe_time / VFRAME_DURATION;
        while (progress >= 1.0) {
            progress -= 1.0;
        }

        const u8 prev_ly = state->gb.ly;
        state->gb.ly = (u8)(progress * GB_LCD_MAX_LY);

        state->gb.stat |= (state->gb.ly == state->gb.lcy) << 2;

        // Trigger some stuff then ly changes
        if (prev_ly != state->gb.ly) {
            // VBlank interrupt
            if (state->gb.ly == 144) {
                state->gb.if_ |= InterruptFlag_VBlank;
            }

            // STAT lcy == ly interrupt
            if ((state->gb.stat & StatSelect_Lyc) != 0 &&
                state->gb.ly == state->gb.lcy) {
                state->gb.if_ |= InterruptFlag_Lcd;
            }
        }

        GameBoy_service_interrupts(&state->gb, &memory);
        Cpu_tick(&state->gb.cpu, &memory);

        // DIV counter
        if (state->gb.cpu.mode != CpuMode_Stopped) {
            state->div_cycle_counter += state->gb.cpu.cycle_count;

            while (state->div_cycle_counter >= DIV_FREQUENCY_CYCLES) {
                ++state->gb.div;
                state->div_cycle_counter -= DIV_FREQUENCY_CYCLES;
            }
        }

        // TIMA is only incremented if TAC's bit 2 is set
        if (state->gb.tac & 0b100) {
            const u8 clock_select = state->gb.tac & 0b11;
            const int tac_delay_cycles =
                clock_select == 0 ? 256 : 4 * clock_select;

            // TIMA counter
            state->tima_cycle_counter += state->gb.cpu.cycle_count;
            if (state->tima_cycle_counter > tac_delay_cycles) {
                state->tima_cycle_counter -= tac_delay_cycles;
                ++state->gb.tima;

                // Trigger timer interrupt when tima overflows
                if (state->gb.tima == 0) {
                    state->gb.tima = state->gb.tma;
                    state->gb.if_ |= InterruptFlag_Timer;
                }
            }
        }

        state->cycle_accumulator += state->gb.cpu.cycle_count;
        state->gb.cpu.cycle_count = 0;
    }

    state->cycle_accumulator -= total_frame_cycles;

    state->vframe_time += delta;
    while (state->vframe_time >= VFRAME_DURATION) {
        state->vframe_time -= VFRAME_DURATION;
    }
}

static void draw_tiles(const State *const state,
                       const SDL_Surface *const surface,
                       const SDL_PixelFormatDetails *const pixel_format)
{
    static constexpr size_t TILES_HORIZONTAL = 32;
    static constexpr size_t TILES_VERTICAL = 32;

    u32 *const pixels = surface->pixels;

    const size_t tile_data_start =
        state->gb.lcdc & LcdControl_BgwTileArea ? 0 : 0x1000;
    const size_t tile_map_start =
        state->gb.lcdc & LcdControl_BgTileMap ? 0x1C00 : 0x1800;

    const u8 *const tile_data = &state->gb.vram[tile_data_start];
    const u8 *const tile_map = &state->gb.vram[tile_map_start];

    for (size_t tile_y = 0; tile_y < TILES_VERTICAL; ++tile_y) {
        for (size_t tile_x = 0; tile_x < TILES_HORIZONTAL; ++tile_x) {
            const u8 tile_index =
                tile_map[(tile_y * TILES_HORIZONTAL) + tile_x];
            const long tile_index_signed =
                state->gb.lcdc & LcdControl_BgwTileArea ? tile_index
                                                        : (i8)tile_index;

            for (size_t tile_row_index = 0; tile_row_index < 8;
                 ++tile_row_index) {
                const u8 byte_1 =
                    tile_data[(tile_index_signed * 16) + (2 * tile_row_index)];
                const u8 byte_2 = tile_data[(tile_index_signed * 16) +
                                            (2 * tile_row_index) + 1];

                for (size_t tile_col_index = 0; tile_col_index < 8;
                     ++tile_col_index) {
                    const u8 bit_lo = (byte_1 >> tile_col_index) & 1;
                    const u8 bit_hi = (byte_2 >> tile_col_index) & 1;
                    const u8 palette_index = bit_lo | (bit_hi << 1);

                    const size_t color =
                        (state->gb.bgp >> (2 * palette_index)) & 0b11;

                    const size_t pixel_y = (8 * tile_y) + tile_row_index;
                    const size_t pixel_x = (8 * tile_x) + 7 - tile_col_index;

                    pixels[(pixel_y * surface->w) + pixel_x] =
                        map_color_index(color, pixel_format);
                }
            }
        }
    }
}

static void draw_objects(const State *const state,
                         const SDL_Surface *const surface,
                         const SDL_PixelFormatDetails *const pixel_format)
{
    u32 *const pixels = surface->pixels;

    for (size_t obj = 0; obj < 40; ++obj) {
        const u8 *const obj_data = &state->gb.oam[obj * 4];

        const size_t y_pos = obj_data[0] - 16;
        const size_t x_pos = obj_data[1] - 8;
        const size_t tile_index = obj_data[2];
        const u8 attrs = obj_data[3];

        // TODO: implement priority (background over object)
        // Will probably need two passes: low and normal priority objs

        const bool flip_x = (attrs & ObjAttrs_FlipX) != 0;
        const bool flip_y = (attrs & ObjAttrs_FlipY) != 0;
        const u8 obp = (attrs & ObjAttrs_DmgPalette) != 0 ? state->gb.obp1
                                                          : state->gb.obp0;

        for (size_t sprite_row = 0; sprite_row < 8; ++sprite_row) {
            // Objects always use the $8000 method
            const u8 byte_1 =
                state->gb.vram[(tile_index * 0x10) + (2 * sprite_row)];
            const u8 byte_2 =
                state->gb.vram[(tile_index * 0x10) + (2 * sprite_row) + 1];

            for (size_t sprite_col = 0; sprite_col < 8; ++sprite_col) {
                const u8 bit_lo = (byte_1 >> sprite_col) & 1;
                const u8 bit_hi = (byte_2 >> sprite_col) & 1;
                const size_t palette_index = bit_lo | (bit_hi << 1);
                const size_t color = (obp >> (palette_index * 2)) & 0b11;

                if (color != 0) {
                    const size_t pixel_y =
                        y_pos + (flip_y ? 7 - sprite_row : sprite_row);
                    const size_t pixel_x =
                        x_pos + (flip_x ? sprite_col : 7 - sprite_col);

                    if (pixel_x < (size_t)surface->w &&
                        pixel_y < (size_t)surface->h) {
                        pixels[(pixel_y * surface->w) + pixel_x] =
                            map_color_index(color, pixel_format);
                    }
                }
            }
        }
    }
}

static void update_texture(const State *const state)
{
    SDL_Surface *surface = nullptr;

    SDL_CHECKED(
        SDL_LockTextureToSurface(state->screen_texture, nullptr, &surface),
        "Could not lock texture");

    const SDL_PixelFormatDetails *const pixel_format =
        SDL_GetPixelFormatDetails(surface->format);
    BAIL_IF(pixel_format == nullptr, "Could not get pixel format: %s",
            SDL_GetError());

    SDL_FillSurfaceRect(surface, nullptr,
                        SDL_MapRGB(pixel_format, nullptr, 0, 0, 0));

    if ((state->gb.lcdc & LcdControl_Enable) != 0) {
        draw_tiles(state, surface, pixel_format);

        if ((state->gb.lcdc & LcdControl_ObjEnable) != 0) {
            draw_objects(state, surface, pixel_format);
        }
    }

    SDL_UnlockTexture(state->screen_texture);
}

static void render(const State *const state, SDL_Renderer *const renderer)
{
    const float ASPECT_RATIO = (float)GB_LCD_WIDTH / GB_LCD_HEIGHT;

    update_texture(state);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // TODO: implement window wrapping
    const SDL_FRect src_rect = {
        .x = state->gb.scx,
        .y = state->gb.scy,
        .w = GB_LCD_WIDTH,
        .h = GB_LCD_HEIGHT,
    };

    const SDL_FRect dest_rect =
        fit_rect_to_aspect_ratio(&(SDL_FRect){0, 0, (float)state->window_width,
                                              (float)state->window_height},
                                 ASPECT_RATIO);

    SDL_RenderTexture(renderer, state->screen_texture, &src_rect, &dest_rect);
    SDL_RenderPresent(renderer);
}

void run_until_quit(State *const state, SDL_Renderer *const renderer)
{
    double last_time = sdl_get_performance_time();
    double time_accumulator = 0.0;

    while (!state->quit) {
        const double frame_start = sdl_get_performance_time();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handle_event(state, &event);
        }

        const double new_time = sdl_get_performance_time();

        time_accumulator += new_time - last_time;
        if (time_accumulator > MAX_TIME_ACCUMULATOR)
            time_accumulator = MAX_TIME_ACCUMULATOR;

        last_time = new_time;

        while (time_accumulator >= DELTA) {
            update(state, DELTA);
            time_accumulator -= DELTA;
        }

        render(state, renderer);

        const double offset = sdl_get_performance_time() - frame_start;
        const double delay = DELTA - offset;

        if (delay > 0) {
            SDL_DelayNS((size_t)(delay * 1e9));
        }
    }
}

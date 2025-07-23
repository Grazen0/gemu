#include "frontend/frontend.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_timer.h"
#include "common/log.h"
#include "core/cpu/cpu.h"
#include "core/game_boy.h"
#include "frontend/sdl.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static constexpr int FPS = 60;
static constexpr double DELTA = 1.0 / FPS;
static constexpr double MAX_TIME_ACCUMULATOR = 4 * DELTA;
static constexpr int DIV_FREQUENCY_HZ = 16384; // 16779 Hz on SGB

static const uint8_t PALETTE_RGB[][3] = {
    {186, 218, 85},
    {130, 153, 59},
    { 74,  87, 34},
    { 19,  22,  8}
};

bool* get_joypad_key(GameBoy* const restrict gb, const SDL_Keycode key) {
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

void handle_event(State* const restrict state, const SDL_Event* const restrict event) {
    switch (event->type) {
        case SDL_EVENT_QUIT: {
            state->quit = true;
            break;
        }
        case SDL_EVENT_WINDOW_RESIZED: {
            state->window_width = event->window.data1;
            state->window_height = event->window.data2;
            break;
        }
        case SDL_EVENT_KEY_DOWN: {
            bool* const state_key = get_joypad_key(&state->gb, event->key.key);
            if (state_key != nullptr)
                *state_key = true;

            break;
        }
        case SDL_EVENT_KEY_UP: {
            if (event->key.mod == SDL_KMOD_CTRL && event->key.key == SDLK_O) {
                // TODO: open new ROM
                break;
            }

            bool* const state_key = get_joypad_key(&state->gb, event->key.key);
            if (state_key != nullptr)
                *state_key = false;

            break;
        }
        default: {
        }
    }
}

void update(State* const restrict state, const double delta) {
    Memory memory = (Memory){
        .ctx = &state->gb,
        .read = GameBoy_read_mem,
        .write = GameBoy_write_mem,
    };

    // log_info(LogCategory_ALL, "========================");
    // log_info(LogCategory_ALL, "if:   %%%08B", state->gb.if_);
    // log_info(LogCategory_ALL, "ie:   %%%08B", state->gb.ie);
    // log_info(LogCategory_ALL, "joyp: %%%08B", state->gb.joyp);
    // log_info(LogCategory_ALL, "stat: %%%08B", state->gb.stat);
    // log_info(LogCategory_ALL, "sc:   %%%08B", state->gb.sc);

    state->gb.cpu.cycle_count = 0;

    const double total_frame_cycles = GB_CPU_FREQUENCY_HZ * delta;

    const double VFRAME_DURATION = 1.0 / GB_VBLANK_FREQ;
    const int DIV_FREQUENCY_CYCLES = GB_CPU_FREQUENCY_HZ / DIV_FREQUENCY_HZ;

    while (state->cycle_accumulator < total_frame_cycles) {
        const double actual_vframe_time =
            state->vframe_time + (state->cycle_accumulator / GB_CPU_FREQUENCY_HZ);
        double progress = actual_vframe_time / VFRAME_DURATION;
        while (progress >= 1.0) {
            progress -= 1.0;
        }

        const uint8_t prev_ly = state->gb.ly;
        state->gb.ly = (uint8_t)(progress * GB_LCD_MAX_LY);

        state->gb.stat |= (state->gb.ly == state->gb.lcy) << 2;

        // Trigger some stuff then ly changes
        if (prev_ly != state->gb.ly) {
            // VBlank interrupt
            if (state->gb.ly == 144) {
                state->gb.if_ |= InterruptFlag_VBLANK;
            }

            // STAT lcy == ly interrupt
            if ((state->gb.stat & StatSelect_LYC) != 0 && state->gb.ly == state->gb.lcy) {
                state->gb.if_ |= InterruptFlag_LCD;
            }
        }

        GameBoy_service_interrupts(&state->gb, &memory);
        Cpu_tick(&state->gb.cpu, &memory);

        // DIV counter
        if (state->gb.cpu.mode != CpuMode_STOPPED) {
            state->div_cycle_counter += state->gb.cpu.cycle_count;

            while (state->div_cycle_counter >= DIV_FREQUENCY_CYCLES) {
                ++state->gb.div;
                state->div_cycle_counter -= DIV_FREQUENCY_CYCLES;
            }
        }

        // TIMA is only incremented if TAC's bit 2 is set
        if (state->gb.tac & 0x04) {
            const uint8_t clock_select = state->gb.tac & 0b11;
            const int tac_delay_cycles = clock_select == 0 ? 256 : 4 * clock_select;

            // TIMA counter
            state->tima_cycle_counter += state->gb.cpu.cycle_count;
            if (state->tima_cycle_counter > tac_delay_cycles) {
                state->tima_cycle_counter -= tac_delay_cycles;
                ++state->gb.tima;

                // Trigger timer interrupt when tima overflows
                if (state->gb.tima == 0) {
                    state->gb.tima = state->gb.tma;
                    state->gb.if_ |= InterruptFlag_TIMER;
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

bool update_texture(const State* const restrict state) {
    SDL_Surface* surface = nullptr;

    if (!SDL_LockTextureToSurface(state->screen_texture, nullptr, &surface)) {
        return false;
    }

    const SDL_PixelFormatDetails* const pixel_format = SDL_GetPixelFormatDetails(surface->format);
    if (pixel_format == nullptr) {
        return false;
    }

    SDL_FillSurfaceRect(surface, nullptr, SDL_MapRGB(pixel_format, nullptr, 0, 0, 0));

    if (state->gb.lcdc & LcdControl_ENABLE) {
        uint32_t* const restrict pixels = surface->pixels;

        const size_t tile_data = state->gb.lcdc & LcdControl_BGW_TILE_AREA ? 0 : 0x1000;
        const size_t tile_map = state->gb.lcdc & LcdControl_BG_TILE_MAP ? 0x1C00 : 0x1800;

        for (size_t tile_y = 0; tile_y < 32; ++tile_y) {
            for (size_t tile_x = 0; tile_x < 32; ++tile_x) {
                const uint8_t tile_index = state->gb.vram[tile_map + (tile_y * 32) + tile_x];
                const long tile_index_signed =
                    state->gb.lcdc & LcdControl_BGW_TILE_AREA ? tile_index : (int8_t)tile_index;

                for (size_t tile_row = 0; tile_row < 8; ++tile_row) {
                    const uint8_t byte_1 =
                        state->gb.vram[tile_data + (tile_index_signed * 16) + (2 * tile_row)];
                    const uint8_t byte_2 =
                        state->gb.vram[tile_data + (tile_index_signed * 16) + (2 * tile_row) + 1];

                    for (size_t tile_col = 0; tile_col < 8; ++tile_col) {
                        const uint8_t bit_lo = (byte_1 >> tile_col) & 1;
                        const uint8_t bit_hi = (byte_2 >> tile_col) & 1;
                        const uint8_t palette_index = bit_lo | (bit_hi << 1);

                        const size_t color = (state->gb.bgp >> (palette_index * 2)) & 0b11;

                        const size_t pixel_y = (tile_y * 8) + tile_row;
                        const size_t pixel_x = (tile_x * 8) + 7 - tile_col;

                        pixels[(pixel_y * surface->w) + pixel_x] = SDL_MapRGB(
                            pixel_format,
                            nullptr,
                            PALETTE_RGB[color][0],
                            PALETTE_RGB[color][1],
                            PALETTE_RGB[color][2]
                        );
                    }
                }
            }
        }

        for (uint16_t i = 0; i < 0xA0; i += 4) {
            const size_t y_pos = state->gb.oam[i] - 16;
            const size_t x_pos = state->gb.oam[i + 1] - 8;
            const size_t tile_index = state->gb.oam[i + 2];
            const uint8_t attrs = state->gb.oam[i + 3];

            typedef enum ObjAttrs : uint8_t {
                ObjAttrs_PRIORITY = 1 << 7,
                ObjAttrs_Y_FLIP = 1 << 6,
                ObjAttrs_X_FLIP = 1 << 5,
                ObjAttrs_DMG_PALETTE = 1 << 4,
                ObjAttrs_BANK = 1 << 3,
                ObjAttrs_CGB_PALETTE = 0b111,
            } ObjAttrs;

            // TODO: implement priority (background over object)
            // Will probably need two passes: low and normal priority objs

            const bool flip_x = (attrs & ObjAttrs_X_FLIP) != 0;
            const bool flip_y = (attrs & ObjAttrs_Y_FLIP) != 0;
            const uint8_t obp =
                (attrs & ObjAttrs_DMG_PALETTE) != 0 ? state->gb.obp1 : state->gb.obp0;

            for (size_t sprite_row = 0; sprite_row < 8; ++sprite_row) {
                const uint8_t byte_1 =
                    state->gb.vram[tile_data + (tile_index * 0x10) + (2 * sprite_row)];
                const uint8_t byte_2 =
                    state->gb.vram[tile_data + (tile_index * 0x10) + (2 * sprite_row) + 1];

                for (size_t sprite_col = 0; sprite_col < 8; ++sprite_col) {
                    const uint8_t bit_lo = (byte_1 >> sprite_col) & 1;
                    const uint8_t bit_hi = (byte_2 >> sprite_col) & 1;
                    const size_t palette_index = bit_lo | (bit_hi << 1);
                    const size_t color = (obp >> (palette_index * 2)) & 0b11;

                    if (color != 0) {
                        const size_t pixel_y = y_pos + (flip_y ? 7 - sprite_row : sprite_row);
                        const size_t pixel_x = x_pos + (flip_x ? sprite_col : 7 - sprite_col);

                        if (pixel_x < (size_t)surface->w && pixel_y < (size_t)surface->h) {
                            pixels[(pixel_y * surface->w) + pixel_x] = SDL_MapRGB(
                                pixel_format,
                                nullptr,
                                PALETTE_RGB[color][0],
                                PALETTE_RGB[color][1],
                                PALETTE_RGB[color][2]
                            );
                        }
                    }
                }
            }
        }
    }

    SDL_UnlockTexture(state->screen_texture);
    return true;
}

void render(const State* const restrict state, SDL_Renderer* const restrict renderer) {
    const double ASPECT_RATIO = (double)GB_LCD_WIDTH / GB_LCD_HEIGHT;

    if (!update_texture(state)) {
        log_warn(LogCategory_KEEP, "Could not update texture: %s", SDL_GetError());
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    SDL_FRect src_rect = {state->gb.scx, state->gb.scy, (float)GB_LCD_WIDTH, (float)GB_LCD_HEIGHT};

    const double real_aspect_ratio = (double)state->window_width / state->window_height;
    SDL_FRect dest_rect;

    if (real_aspect_ratio == ASPECT_RATIO) {
        // Window is exactly the right aspect ratio
        dest_rect.w = (float)state->window_width;
        dest_rect.h = (float)state->window_height;
        dest_rect.x = 0.0F;
        dest_rect.y = 0.0F;
    } else if (real_aspect_ratio > ASPECT_RATIO) {
        // Window is stretched out horizontally
        dest_rect.w = (float)(state->window_height * ASPECT_RATIO);
        dest_rect.h = (float)state->window_height;
        dest_rect.x = ((float)state->window_width / 2.0F) - (dest_rect.w / 2.0F);
        dest_rect.y = 0.0F;
    } else {
        // Window is stretched out vertically
        dest_rect.w = (float)state->window_width;
        dest_rect.h = (float)(state->window_width / ASPECT_RATIO);
        dest_rect.x = 0.0F;
        dest_rect.y = ((float)state->window_height / 2.0F) - (dest_rect.h / 2.0F);
    }

    SDL_RenderTexture(renderer, state->screen_texture, &src_rect, &dest_rect);
    SDL_RenderPresent(renderer);
}

void run_until_quit(State* const restrict state, SDL_Renderer* const restrict renderer) {
    double last_time = sdl_get_performance_time();
    double time_accumulator = 0.0;

    while (!state->quit) {
        const double frame_start = sdl_get_performance_time();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handle_event(state, &event);
        }

        const double new_time = sdl_get_performance_time();

        time_accumulator = fmin(time_accumulator + new_time - last_time, MAX_TIME_ACCUMULATOR);
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

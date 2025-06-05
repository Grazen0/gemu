#include "frontend/frontend.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_timer.h"
#include "common/log.h"
#include "core/cpu.h"
#include "core/game_boy.h"
#include "frontend/sdl.h"

static const int FPS = 60;
static const double DELTA = 1.0 / FPS;
static const double MAX_TIME_ACCUMULATOR = 4 * DELTA;
static const int DIV_FREQUENCY_HZ = 16384;  // 16779 Hz on SGB

static const uint8_t PALETTE_RGB[][3] = {
    {186, 218, 85},
    {130, 153, 59},
    { 74,  87, 34},
    { 19,  22,  8}
};

bool* get_joypad_key(State* const restrict state, const SDL_Keycode key) {
    switch (key) {
        case SDLK_RETURN:
            return &state->joypad.start;
        case SDLK_SPACE:
            return &state->joypad.select;
        case SDLK_UP:
            return &state->joypad.up;
        case SDLK_DOWN:
            return &state->joypad.down;
        case SDLK_RIGHT:
            return &state->joypad.right;
        case SDLK_LEFT:
            return &state->joypad.left;
        case SDLK_Z:
            return &state->joypad.a;
        case SDLK_X:
            return &state->joypad.b;
        default:
            return NULL;
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
            if (event->key.mod != 0) {
                break;
            }

            bool* const state_key = get_joypad_key(state, event->key.key);
            if (state_key != NULL) {
                *state_key = true;
            }
            break;
        }
        case SDL_EVENT_KEY_UP: {
            if (event->key.mod != 0) {
                break;
            }

            bool* const state_key = get_joypad_key(state, event->key.key);
            if (state_key != NULL) {
                *state_key = false;
            }
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

    // Update JOYP
    // TODO: only update when necessary
    const uint8_t prev_joyp = state->gb.joyp;
    state->gb.joyp |= 0x0F;

    if ((state->gb.joyp & Joypad_D_PAD_SELECT) == 0) {
        if (state->joypad.right) {
            state->gb.joyp &= ~Joypad_A_RIGHT;
        }
        if (state->joypad.left) {
            state->gb.joyp &= ~Joypad_B_LEFT;
        }
        if (state->joypad.up) {
            state->gb.joyp &= ~Joypad_SELECT_UP;
        }
        if (state->joypad.down) {
            state->gb.joyp &= ~Joypad_START_DOWN;
        }
    }
    if ((state->gb.joyp & Joypad_BUTTONS_SELECT) == 0) {
        if (state->joypad.a) {
            state->gb.joyp &= ~Joypad_A_RIGHT;
        }
        if (state->joypad.b) {
            state->gb.joyp &= ~Joypad_B_LEFT;
        }
        if (state->joypad.select) {
            state->gb.joyp &= ~Joypad_SELECT_UP;
        }
        if (state->joypad.start) {
            state->gb.joyp &= ~Joypad_START_DOWN;
        }
    }

    // Trigger joypad interrupt if necessary
    if ((prev_joyp & ~state->gb.joyp & 0xF) != 0) {
        state->gb.if_ |= InterruptFlag_JOYPAD;
    }

    log_info(LogCategory_ALL, "========================");
    log_info(LogCategory_ALL, "if:   %%%08B", state->gb.if_);
    log_info(LogCategory_ALL, "ie:   %%%08B", state->gb.ie);
    log_info(LogCategory_ALL, "joyp: %%%08B", state->gb.joyp);
    log_info(LogCategory_ALL, "stat: %%%08B", state->gb.stat);
    log_info(LogCategory_ALL, "sc:   %%%08B", state->gb.sc);
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

        // Trigger VBLANK when ly changes to 144
        if (prev_ly != state->gb.ly) {
            if (state->gb.ly == 144) {
                state->gb.if_ |= InterruptFlag_VBLANK;
            }

            if ((state->gb.stat & StatSelect_LYC) != 0 && state->gb.ly == state->gb.lcy) {
                state->gb.if_ |= InterruptFlag_LCD;
            }
        }

        GameBoy_service_interrupts(&state->gb, &memory);
        Cpu_tick(&state->gb.cpu, &memory);

        // DIV counter
        // TODO: should not increment when in STOP mode
        state->div_cycle_counter += state->gb.cpu.cycle_count;
        while (state->div_cycle_counter >= DIV_FREQUENCY_CYCLES) {
            ++state->gb.div;
            state->div_cycle_counter -= DIV_FREQUENCY_CYCLES;
        }

        // TIMA is only incremented if TAC's bit 2 is set
        if (state->gb.tac & 0x04) {
            const uint8_t clock_select = state->gb.tac & 0x03;
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
    SDL_Surface* surface = NULL;

    if (!SDL_LockTextureToSurface(state->screen_texture, NULL, &surface)) {
        return false;
    }

    const SDL_PixelFormatDetails* const pixel_format = SDL_GetPixelFormatDetails(surface->format);

    if (pixel_format == NULL) {
        return false;
    }

    SDL_FillSurfaceRect(surface, NULL, SDL_MapRGB(pixel_format, NULL, 0, 0, 0));

    if (state->gb.lcdc & LcdControl_ENABLE) {
        uint32_t* const restrict pixels = surface->pixels;

        const size_t tile_data = state->gb.lcdc & LcdControl_BGW_TILE_AREA ? 0 : 0x1000;
        const size_t tile_map = state->gb.lcdc & LcdControl_BG_TILE_MAP ? 0x1C00 : 0x1800;

        for (size_t tile_y = 0; tile_y < 32; ++tile_y) {
            for (size_t tile_x = 0; tile_x < 32; ++tile_x) {
                const uint8_t tile_index = state->gb.vram[tile_map + (tile_y * 32) + tile_x];
                const long tile_index_signed =
                    state->gb.lcdc & LcdControl_BGW_TILE_AREA ? tile_index : (int8_t)tile_index;

                for (size_t py = 0; py < 8; ++py) {
                    const uint8_t byte_1 =
                        state->gb.vram[tile_data + (tile_index_signed * 16) + (2 * py)];
                    const uint8_t byte_2 =
                        state->gb.vram[tile_data + (tile_index_signed * 16) + (2 * py) + 1];

                    for (size_t px = 0; px < 8; ++px) {
                        const uint8_t bit_lo = (byte_1 >> px) & 1;
                        const uint8_t bit_hi = (byte_2 >> px) & 1;
                        const uint8_t palette_index = bit_lo | (bit_hi << 1);

                        const uint8_t color = (state->gb.bgp >> (palette_index * 2)) & 0x3;

                        const size_t pixel_y = (tile_y * 8) + py;
                        const size_t pixel_x = (tile_x * 8) + 7 - px;

                        pixels[(pixel_y * surface->w) + pixel_x] = SDL_MapRGB(
                            pixel_format,
                            NULL,
                            PALETTE_RGB[color][0],
                            PALETTE_RGB[color][1],
                            PALETTE_RGB[color][2]
                        );
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

        time_accumulator += new_time - last_time;
        if (time_accumulator > MAX_TIME_ACCUMULATOR) {
            time_accumulator = MAX_TIME_ACCUMULATOR;
        }
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

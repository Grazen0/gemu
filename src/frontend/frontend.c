#include "frontend.h"
#include <stddef.h>
#include <stdint.h>
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_timer.h"
#include "common/log.h"
#include "core/cpu.h"
#include "core/game_boy.h"

static const int FPS = 60;
static const double DELTA = 1.0 / FPS;

static const uint8_t PALETTE_RGB[][3] = {
    {186, 218, 85},
    {130, 153, 59},
    { 74,  87, 34},
    { 19,  22,  8}
};

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
        default: {
        }
    }
}

void trigger_interrupts(GameBoy* const restrict gb, Memory* const restrict mem) {
    if (!gb->cpu.ime) {
        return;
    }

    const uint8_t int_mask = gb->iff & gb->ie;

    for (uint8_t i = 0; i <= 4; i++) {
        if (int_mask & (1 << i)) {
            log_info("Interrupt #%i", i);
            gb->iff &= ~(1 << i);
            Cpu_interrupt(&gb->cpu, mem, 0x40 | (i << 3));
            break;
        }
    }
}

void update(State* const restrict state, const double delta) {
    Memory mem = (Memory){
        .ctx = &state->gb,
        .read = GameBoy_read_mem,
        .write = GameBoy_write_mem,
    };

    const double frame_cycles = GB_CPU_FREQ_M * delta;
    state->gb.cpu.cycle_count = 0;

    bool is_vblank_pending = true;

    while (state->gb.cpu.cycle_count < frame_cycles) {
        const double progress = (double)state->gb.cpu.cycle_count / frame_cycles;
        state->gb.ly = (uint8_t)(progress * GB_LCD_MAX_LY);

        if (is_vblank_pending && state->gb.ly == 144) {
            state->gb.iff |= 1;
            is_vblank_pending = false;
        }

        trigger_interrupts(&state->gb, &mem);
        Cpu_tick(&state->gb.cpu, &mem);
    }
}

bool update_texture(const State* const restrict state) {
    SDL_Surface* surface = NULL;

    if (!SDL_LockTextureToSurface(state->texture, NULL, &surface)) {
        return false;
    }

    const SDL_PixelFormatDetails* const pixel_format = SDL_GetPixelFormatDetails(surface->format);

    if (pixel_format == NULL) {
        return false;
    }

    SDL_FillSurfaceRect(surface, NULL, SDL_MapRGB(pixel_format, NULL, 0, 0, 0));

    if (state->gb.lcdc & LCDC_ENABLE) {
        uint32_t* const restrict pixels = surface->pixels;

        const size_t tile_data = state->gb.lcdc & LCDC_BGW_TILE_AREA ? 0 : 0x1000;
        const size_t tile_map = state->gb.lcdc & LCDC_BG_TILE_MAP ? 0x1C00 : 0x1800;

        for (size_t tile_y = 0; tile_y < 32; tile_y++) {
            for (size_t tile_x = 0; tile_x < 32; tile_x++) {
                const uint8_t tile_index = state->gb.vram[tile_map + (tile_y * 32) + tile_x];
                const long tile_index_signed =
                    state->gb.lcdc & LCDC_BGW_TILE_AREA ? tile_index : (int8_t)tile_index;

                for (size_t py = 0; py < 8; py++) {
                    const uint8_t byte_1 =
                        state->gb.vram[tile_data + (tile_index_signed * 16) + (2 * py)];
                    const uint8_t byte_2 =
                        state->gb.vram[tile_data + (tile_index_signed * 16) + (2 * py) + 1];

                    for (size_t px = 0; px < 8; px++) {
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

    SDL_UnlockTexture(state->texture);
    return true;
}

void render(const State* const restrict state, SDL_Renderer* const restrict renderer) {
    const double ASPECT_RATIO = (double)GB_LCD_WIDTH / GB_LCD_HEIGHT;

    if (!update_texture(state)) {
        log_warn("Could not update texture: %s", SDL_GetError());
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

    SDL_RenderTexture(renderer, state->texture, &src_rect, &dest_rect);
    SDL_RenderPresent(renderer);
}

void frame(State* const restrict state, SDL_Renderer* const restrict renderer) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        handle_event(state, &event);
    }

    const long double new_time = (long double)SDL_GetTicks() / 1000;
    state->time_accumulator += new_time - state->current_time;
    state->current_time = new_time;

    while (state->time_accumulator >= DELTA) {
        update(state, DELTA);
        state->time_accumulator -= DELTA;
    }

    render(state, renderer);
    SDL_DelayNS((size_t)(DELTA * 1e9));
}

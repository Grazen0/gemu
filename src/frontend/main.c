#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include "common/control.h"
#include "common/log.h"
#include "core/data.h"
#include "core/game_boy.h"
#include "frontend.h"

void sdl_check(const bool result, const char* const restrict message) {
    if (!result) {
        log_error("%s: %s", message, SDL_GetError());
        exit(1);
    }
}

int main(const int argc, const char* const argv[]) {
    const int WINDOW_WIDTH_INITIAL = GB_LCD_WIDTH * 4;
    const int WINDOW_HEIGHT_INITIAL = GB_LCD_HEIGHT * 4;

    if (argc < 2) {
        log_error("No ROM filename specified.");
        return 1;
    }

    size_t rom_len = 0;
    uint8_t* const rom = SDL_LoadFile(argv[1], &rom_len);

    bail_if(rom == NULL, "Could not read ROM file.");
    bail_if(
        rom_len != 0x8000 * ((size_t)1 << (size_t)rom[ROM_ROM_SIZE]),
        "ROM length does not match header info."
    );

    log_info("Cartridge type: 0x%02X", rom[ROM_CARTRIDGE_TYPE]);

    sdl_check(SDL_Init(SDL_INIT_VIDEO), "Could not initialize video\n");

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    sdl_check(
        SDL_CreateWindowAndRenderer(
            "gemu", WINDOW_WIDTH_INITIAL, WINDOW_HEIGHT_INITIAL, 0, &window, &renderer
        ),
        "Could not create window or renderer"
    );

    SDL_Texture* const restrict texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, GB_BG_WIDTH, GB_BG_HEIGHT
    );
    sdl_check(texture != NULL, "Could not create texture");

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    State state = {
        .gb = GameBoy_new(rom, rom_len),
        .window_width = WINDOW_WIDTH_INITIAL,
        .window_height = WINDOW_HEIGHT_INITIAL,
        .current_time = SDL_GetTicks() / 1000.0L,
        .time_accumulator = 0,
        .quit = false,
        .texture = texture,
    };

    SDL_RenderPresent(renderer);
    SDL_SetWindowResizable(window, true);

    while (!state.quit) {
        frame(&state, renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(state.texture);
    SDL_Quit();

    GameBoy_destroy(&state.gb);

    return 0;
}

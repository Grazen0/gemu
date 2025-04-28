#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_video.h"
#include "core/data.h"
#include "core/game_boy.h"
#include "frontend.h"
#include "lib/log.h"

#define WINDOW_WIDTH_INITIAL (GB_LCD_WIDTH * 4)
#define WINDOW_HEIGHT_INITIAL (GB_LCD_HEIGHT * 4)

#define FPS 60

#define SDL_CHECK(result, message)                                               \
    if (!(result)) {                                                             \
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s: %s", message, SDL_GetError()); \
        exit(1);                                                                 \
    }

int main(const int argc, const char* const argv[]) {
    if (argc < 2) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No ROM filename specified.");
        return 1;
    }

    size_t rom_len = 0;
    uint8_t* const rom = SDL_LoadFile(argv[1], &rom_len);

    BAIL_IF(rom == NULL, "Could not read ROM file.");
    BAIL_IF(
        rom_len != 0x8000 * ((size_t)1 << (size_t)rom[ROM_ROM_SIZE]),
        "ROM length does not match header info."
    );

    SDL_Log("Cartridge type: 0x%02X", rom[ROM_CARTRIDGE_TYPE]);

    SDL_CHECK(SDL_Init(SDL_INIT_VIDEO), "Could not initialize video");

    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    SDL_CHECK(
        SDL_CreateWindowAndRenderer(
            "gemu", WINDOW_WIDTH_INITIAL, WINDOW_HEIGHT_INITIAL, 0, &window, &renderer
        ),
        "Could not create window or renderer"
    );

    SDL_Texture* const restrict texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, GB_BG_WIDTH, GB_BG_HEIGHT
    );
    SDL_CHECK(texture != NULL, "Could not create texture");

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    State state = {
        .gb = GameBoy_new(rom, rom_len),
        .window_width = WINDOW_WIDTH_INITIAL,
        .window_height = WINDOW_HEIGHT_INITIAL,
        .current_time = SDL_GetTicks() / 1000.0,
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

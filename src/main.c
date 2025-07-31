#include "SDL3/SDL_error.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_video.h"
#include "boot_rom.h"
#include "control.h"
#include "data.h"
#include "frontend.h"
#include "game_boy.h"
#include "log.h"
#include "string.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SDL_CHECK(result, message) BAIL_IF(!(result), "%s: %s", message, SDL_GetError());

int main(const int argc, const char* const argv[]) {
    const int log_init_result = logger_init(
        LogCategory_All & ~LogCategory_Memory & ~LogCategory_Instruction & ~LogCategory_Interrupt
    );
    if (log_init_result != 0) {
        fprintf(stderr, "Could not create logger thread. Error code %i", log_init_result);
    }

    static constexpr int WINDOW_WIDTH_INITIAL = GB_LCD_WIDTH * 4;
    static constexpr int WINDOW_HEIGHT_INITIAL = GB_LCD_HEIGHT * 4;

    if (argc < 2) {
        log_error(LogCategory_Keep, "No ROM filename specified.");
        return 1;
    }

    size_t rom_len = 0;
    uint8_t* const rom = SDL_LoadFile(argv[1], &rom_len);

    BAIL_IF(rom == nullptr, "Could not read ROM file.");
    BAIL_IF(
        rom_len != 0x8000 * ((size_t)1 << rom[RomHeader_RomSize]),
        "ROM length does not match header info."
    );

    log_info(LogCategory_Keep, "Cartridge type: $%02X", rom[RomHeader_CartridgeType]);

    char game_title[0x11];
    SDL_strlcpy(game_title, (char*)&rom[RomHeader_Title], sizeof(game_title));
    log_info(LogCategory_Keep, "Game title: %s", game_title);

    SDL_CHECK(SDL_Init(SDL_INIT_VIDEO), "Could not initialize video\n");

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    SDL_CHECK(
        SDL_CreateWindowAndRenderer(
            "gemu", WINDOW_WIDTH_INITIAL, WINDOW_HEIGHT_INITIAL, 0, &window, &renderer
        ),
        "Could not create window or renderer"
    );

    SDL_Texture* const restrict texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, GB_BG_WIDTH, GB_BG_HEIGHT
    );
    SDL_CHECK(texture != nullptr, "Could not create texture");

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    State state = (State){
        .gb = GameBoy_new(BOOT_ROM, rom, rom_len),
        .window_width = WINDOW_WIDTH_INITIAL,
        .window_height = WINDOW_HEIGHT_INITIAL,
        .cycle_accumulator = 0.0,
        .vframe_time = 0.0,
        .div_cycle_counter = 0,
        .tima_cycle_counter = 0,
        .quit = false,
        .screen_texture = texture,
    };

    SDL_RenderPresent(renderer);
    SDL_SetWindowResizable(window, true);

    run_until_quit(&state, renderer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(state.screen_texture);
    SDL_Quit();

    GameBoy_destroy(&state.gb);

    return 0;
}

#include "SDL3/SDL_init.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_video.h"
#include "argparse.h"
#include "data.h"
#include "frontend.h"
#include "game_boy.h"
#include "log.h"
#include "sdl.h"
#include "string.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static constexpr int WINDOW_WIDTH_INITIAL = GB_LCD_WIDTH * 4;
static constexpr int WINDOW_HEIGHT_INITIAL = GB_LCD_HEIGHT * 4;

static const char* const usages[] = {
    "gemu [options] [--] <path-to-rom>",
    nullptr,
};

int main(int argc, const char* argv[]) {
    logger_init(
        LogCategory_All & ~LogCategory_Memory & ~LogCategory_Instruction & ~LogCategory_Interrupt
    );

    const char* boot_rom_path = nullptr;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('b', "boot-rom", (void*)&boot_rom_path, "path to boot ROM", nullptr, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse, "A Game Boy emulator written in C.", nullptr);

    argc = argparse_parse(&argparse, argc, argv);

    if (argc < 1) {
        argparse_usage(&argparse);
        return 1;
    }

    size_t rom_len = 0;
    uint8_t* const rom = SDL_LoadFile(argv[0], &rom_len);

    if (rom == nullptr) {
        log_error(LogCategory_Keep, "Could not read ROM file.");
        return 1;
    }

    if (rom_len != 0x8000 * ((size_t)1 << rom[RomHeader_RomSize])) {
        log_error(LogCategory_Keep, "ROM length does not match header info.");
        return 1;
    }

    log_info(LogCategory_Keep, "Cartridge type: $%02X", rom[RomHeader_CartridgeType]);

    char game_title[17];
    SDL_strlcpy(game_title, (char*)&rom[RomHeader_Title], sizeof(game_title));
    log_info(LogCategory_Keep, "Game title: %s", game_title);

    SDL_CHECKED(SDL_Init(SDL_INIT_VIDEO), "Could not initialize video\n");

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    SDL_CHECKED(
        SDL_CreateWindowAndRenderer(
            "gemu", WINDOW_WIDTH_INITIAL, WINDOW_HEIGHT_INITIAL, 0, &window, &renderer
        ),
        "Could not create window or renderer"
    );

    SDL_Texture* const restrict texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, GB_BG_WIDTH, GB_BG_HEIGHT
    );
    SDL_CHECKED(texture != nullptr, "Could not create texture");

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    uint8_t* boot_rom = nullptr;

    if (boot_rom_path != nullptr) {
        size_t boot_rom_len = 0;
        boot_rom = SDL_LoadFile(boot_rom_path, &boot_rom_len);

        if (boot_rom == nullptr) {
            log_error(LogCategory_Keep, "Could not read boot ROM file.");
            return 1;
        }

        if (boot_rom_len != GB_BOOT_ROM_LEN_EXPECTED) {
            log_error(
                LogCategory_Keep,
                "Boot ROM must be exactly %zu bytes long (was %zu)",
                GB_BOOT_ROM_LEN_EXPECTED,
                boot_rom_len
            );
            SDL_free(boot_rom);
            return 1;
        }
    }

    State state = (State){
        .gb = GameBoy_new(boot_rom, rom, rom_len),
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

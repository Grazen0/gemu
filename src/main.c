#include "frontend.h"
#include "game_boy.h"
#include "log.h"
#include "sdl.h"
#include "stdinc.h"
#include "string.h"
#include <SDL3/SDL.h>
#include <argparse.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static constexpr int WINDOW_WIDTH_INITIAL = GB_LCD_WIDTH * 4;
static constexpr int WINDOW_HEIGHT_INITIAL = GB_LCD_HEIGHT * 4;

static const char *const usages[] = {
    "gemu [options] [--] <path-to-rom>",
    nullptr,
};

int main(int argc, const char *argv[])
{
    atexit(SDL_Quit);

    const char *boot_rom_path = nullptr;
    const char *log_level_str = nullptr;

    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('b', "boot-rom", (void *)&boot_rom_path, "path to boot ROM",
                   nullptr, 0, 0),
        OPT_STRING('l', "log-level", (void *)&log_level_str,
                   "log level (one of trace, debug, info, warn, error)",
                   nullptr, 0, 0),
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

    LogLevel log_level = LOG_INFO;

    if (log_level_str != nullptr &&
        !LogLevel_from_str(log_level_str, &log_level)) {
        argparse_usage(&argparse);
        return 1;
    }

    logger_init(log_level);

    size_t rom_len = 0;
    u8 *rom = SDL_LoadFile(argv[0], &rom_len);
    SDL_CHECKED(rom != nullptr, "Could not read ROM file");

    SDL_CHECKED(SDL_Init(SDL_INIT_VIDEO), "Could not initialize video");

    SDL_Window *window = SDL_CreateWindow("gemu", WINDOW_WIDTH_INITIAL,
                                          WINDOW_HEIGHT_INITIAL, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);

    assert(window);
    assert(renderer);

    SDL_PropertiesID props = SDL_GetRendererProperties(renderer);

    const char *name =
        SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, "unknown");

    printf("renderer: %s\n", name);
    return 0;

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             GB_BG_WIDTH, GB_BG_HEIGHT);
    SDL_CHECKED(texture != nullptr, "Could not create texture");

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    u8 *boot_rom = nullptr;

    if (boot_rom_path != nullptr) {
        size_t boot_rom_len = 0;
        boot_rom = SDL_LoadFile(boot_rom_path, &boot_rom_len);
        SDL_CHECKED(boot_rom != nullptr, "Could not read boot ROM file.");

        if (boot_rom_len != GB_BOOT_ROM_LEN) {
            log_error("Boot ROM must be exactly %zu bytes long (was %zu)",
                      GB_BOOT_ROM_LEN, boot_rom_len);
            SDL_free(boot_rom);
            return 1;
        }
    }

    State state = {
        .gb = GameBoy_new(boot_rom),
        .window_width = WINDOW_WIDTH_INITIAL,
        .window_height = WINDOW_HEIGHT_INITIAL,
        .cycle_accumulator = 0.0,
        .vframe_time = 0.0,
        .div_cycle_counter = 0,
        .tima_cycle_counter = 0,
        .quit = false,
        .screen_texture = texture,
    };

    GameBoy_load_rom(&state.gb, rom, rom_len);

    SDL_free(boot_rom);
    SDL_free(rom);

    GameBoy_log_cartridge_info(&state.gb);

    SDL_RenderPresent(renderer);
    SDL_SetWindowResizable(window, true);

    run_until_quit(&state, renderer);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(state.screen_texture);

    GameBoy_destroy(&state.gb);

    return 0;
}

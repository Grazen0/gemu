#include "frontend.h"
#include "game_boy.h"
#include "log.h"
#include "stdinc.h"
#include "string.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static constexpr int WINDOW_INIT_WIDTH = GB_LCD_WIDTH * 4;
static constexpr int WINDOW_INIT_HEIGHT = GB_LCD_HEIGHT * 4;

#define EXE_NAME "gemu"

static constexpr char USAGE[] =
    "Usage: " EXE_NAME " [options] [--] [file]                               \n"
    "A Game Boy emulator written in C.                                       \n"
    "                                                                        \n"
    "  -h, --help                 show this help message                     \n"
    "  -b, --boot-rom=FILE        boot ROM to use for startup                \n"
    "  -l, --log-level=LOG_LEVEL  log level (error, warn, info, debug, trace)\n";

static const struct option OPTIONS[] = {
    {     "help",       no_argument, nullptr, 'h'},
    { "boot-rom", required_argument, nullptr, 'b'},
    {"log-level", required_argument, nullptr, 'l'},
};

typedef struct {
    bool help;
    const char *rom_path;
    const char *boot_rom_path;
    LogLevel log_level;
} Args;

static Args args_init()
{
    return (Args){
        .help = false,
        .rom_path = nullptr,
        .boot_rom_path = nullptr,
        .log_level = LOG_LEVEL_INFO,
    };
}

static bool parse_args(int argc, char **argv, Args *out_args)
{
    *out_args = args_init();

    const char *log_level_str = nullptr;
    int opt = -1;

    while ((opt = getopt_long(argc, argv, "hb:l:", OPTIONS, nullptr)) != -1) {
        switch (opt) {
            case 'h':
                out_args->help = true;
                return true;
            case 'b':
                out_args->boot_rom_path = optarg;
                break;
            case 'l':
                log_level_str = optarg;
                break;
            default:
                return false;
        }
    }

    if (log_level_str != nullptr &&
        !log_level_from_str(log_level_str, &out_args->log_level))
        return false;

    if (optind >= argc)
        return false;

    out_args->rom_path = argv[optind];
    return true;
}

int main(int argc, char *argv[])
{
    Args args = {};

    if (!parse_args(argc, argv, &args)) {
        fputs(USAGE, stderr);
        printf("Try '" EXE_NAME " -h' for more information.\n");
        return EXIT_FAILURE;
    }

    if (args.help) {
        fputs(USAGE, stdout);
        return EXIT_SUCCESS;
    }

    int retval = EXIT_SUCCESS;

    size_t rom_len = 0;
    u8 *rom = SDL_LoadFile(args.rom_path, &rom_len);

    if (rom == nullptr) {
        fprintf(stderr, "%s\n", SDL_GetError());
        retval = EXIT_FAILURE;
        goto cleanup_1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Could not read initialize video: %s\n",
                SDL_GetError());
        retval = EXIT_FAILURE;
        goto cleanup_1;
    }

    atexit(SDL_Quit);
    logger_set_level(args.log_level);

    SDL_Window *window =
        SDL_CreateWindow("gemu", WINDOW_INIT_WIDTH, WINDOW_INIT_HEIGHT, 0);

    if (window == nullptr) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        retval = EXIT_FAILURE;
        goto cleanup_2;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);

    if (renderer == nullptr) {
        fprintf(stderr, "Could not create renderer: %s\n", SDL_GetError());
        retval = EXIT_FAILURE;
        goto cleanup_2;
    }

    SDL_PropertiesID props = SDL_GetRendererProperties(renderer);

    const char *renderer_name =
        SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, "unknown");
    log_info("Using renderer \"%s\"", renderer_name);

    u8 *boot_rom = nullptr;

    if (args.boot_rom_path != nullptr) {
        size_t boot_rom_len = 0;
        boot_rom = SDL_LoadFile(args.boot_rom_path, &boot_rom_len);

        if (boot_rom == nullptr) {
            fprintf(stderr, "Could not read boot ROM file.\n");
            retval = EXIT_FAILURE;
            goto cleanup_3;
        }

        if (boot_rom_len != GB_BOOT_ROM_LEN) {
            fprintf(stderr,
                    "Boot ROM must be exactly %zu bytes long (was %zu)\n",
                    GB_BOOT_ROM_LEN, boot_rom_len);
            retval = EXIT_FAILURE;
            goto cleanup_4;
        }
    }

    GameInfo info = gb_cartridge_info(rom);
    log_info("Cartridge type: $%02X", info.cart_type);
    log_info("RAM size: $%02X", info.ram_size);
    log_info("ROM size: $%02X", info.rom_size);
    log_info("Game title: %s", info.title);

    State state = state_init(boot_rom, window);
    gb_load_rom(&state.gb, rom, rom_len);

    SDL_RenderPresent(renderer);
    SDL_SetWindowResizable(window, true);
    run_until_quit(&state, renderer);

    state_deinit(&state);
cleanup_4:
    SDL_free(boot_rom);
cleanup_3:
    SDL_DestroyRenderer(renderer);
cleanup_2:
    SDL_DestroyWindow(window);
cleanup_1:
    SDL_free(rom);

    return retval;
}

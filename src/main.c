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

static constexpr int WINDOW_WIDTH_INITIAL = GB_LCD_WIDTH * 4;
static constexpr int WINDOW_HEIGHT_INITIAL = GB_LCD_HEIGHT * 4;

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
        .log_level = LOG_INFO,
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

    logger_set_level(args.log_level);

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

    SDL_Window *window = SDL_CreateWindow("gemu", WINDOW_WIDTH_INITIAL,
                                          WINDOW_HEIGHT_INITIAL, 0);

    if (window == nullptr) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        retval = EXIT_FAILURE;
        goto cleanup_2;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);

    if (renderer == nullptr) {
        fprintf(stderr, "Could not create renderer: %s\n", SDL_GetError());
        retval = EXIT_FAILURE;
        goto cleanup_3;
    }

    SDL_PropertiesID props = SDL_GetRendererProperties(renderer);

    const char *name =
        SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, "unknown");
    log_info("Renderer: %s", name);

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             GB_BG_WIDTH, GB_BG_HEIGHT);

    if (texture == nullptr) {
        fprintf(stderr, "Could not create texture: %s\n", SDL_GetError());
        retval = EXIT_FAILURE;
        goto cleanup_4;
    }

    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    u8 *boot_rom = nullptr;

    if (args.boot_rom_path != nullptr) {
        size_t boot_rom_len = 0;
        boot_rom = SDL_LoadFile(args.boot_rom_path, &boot_rom_len);

        if (boot_rom == nullptr) {
            fprintf(stderr, "Could not read boot ROM file.\n");
            retval = EXIT_FAILURE;
            goto cleanup_5;
        }

        if (boot_rom_len != GB_BOOT_ROM_LEN) {
            fprintf(stderr,
                    "Boot ROM must be exactly %zu bytes long (was %zu)\n",
                    GB_BOOT_ROM_LEN, boot_rom_len);
            retval = EXIT_FAILURE;
            goto cleanup_6;
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

    GameBoy_log_cartridge_info(&state.gb);

    SDL_RenderPresent(renderer);
    SDL_SetWindowResizable(window, true);

    run_until_quit(&state, renderer);

    GameBoy_destroy(&state.gb);

cleanup_6:
    SDL_free(boot_rom);
cleanup_5:
    SDL_DestroyTexture(texture);
cleanup_4:
    SDL_DestroyRenderer(renderer);
cleanup_3:
    SDL_DestroyWindow(window);
cleanup_2:
    SDL_Quit();
cleanup_1:
    SDL_free(rom);

    return retval;
}

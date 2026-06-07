#include "frontend.h"
#include "game_boy.h"
#include "log.h"
#include "stdinc.h"
#include "string.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static constexpr int WINDOW_INIT_WIDTH = GB_LCD_WIDTH * 4;
static constexpr int WINDOW_INIT_HEIGHT = GB_LCD_HEIGHT * 4;

#define EXE_NAME "gemu"

static constexpr char USAGE[] =
    "Usage: " EXE_NAME " [options] [rom_file]                                \n"
    "A Game Boy emulator written in C.                                       \n"
    "                                                                        \n"
    "  -h, --help                 show this help message                     \n"
    "  -b, --boot-rom FILE        boot ROM to use for startup                \n"
    "  -l, --log-level LOG_LEVEL  log level (error, warn, info, debug, trace)\n";

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

u8 *load_file(const char filename[], size_t *data_size)
{
    FILE *file = fopen(filename, "r");
    if (file == nullptr)
        return nullptr;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    u8 *data = calloc(size, sizeof(*data));
    if (data == nullptr)
        return nullptr;

    fread(data, sizeof(*data), size, file);

    if (data_size != nullptr)
        *data_size = size;

    return data;
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

    size_t rom_len = 0;
    u8 *rom = load_file(args.rom_path, &rom_len);

    if (rom == nullptr) {
        log_error("%s", SDL_GetError());
        return EXIT_FAILURE;
    }

    int retval = EXIT_SUCCESS;
    u8 *boot_rom = nullptr;

    if (args.boot_rom_path != nullptr) {
        size_t boot_rom_len = 0;
        boot_rom = load_file(args.boot_rom_path, &boot_rom_len);

        if (boot_rom == nullptr) {
            log_error("Could not read boot ROM file: %s", strerror(errno));
            retval = EXIT_FAILURE;
            goto cleanup_1;
        }

        if (boot_rom_len != GB_BOOT_ROM_LEN) {
            log_error("Boot ROM must be exactly %zu bytes long (was %zu)",
                      GB_BOOT_ROM_LEN, boot_rom_len);
            retval = EXIT_FAILURE;
            goto cleanup_2;
        }
    }

    GameBoy gb = gb_init(boot_rom);
    gb_load_rom(&gb, rom, rom_len);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        log_error("Could not read initialize video: %s", SDL_GetError());
        retval = EXIT_FAILURE;
        goto cleanup_3;
    }

    atexit(SDL_Quit);

    SDL_Window *window =
        SDL_CreateWindow("gemu", WINDOW_INIT_WIDTH, WINDOW_INIT_HEIGHT, 0);

    if (window == nullptr) {
        log_error("Could not create window: %s", SDL_GetError());
        retval = EXIT_FAILURE;
        goto cleanup_3;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);

    if (renderer == nullptr) {
        log_error("Could not create renderer: %s", SDL_GetError());
        retval = EXIT_FAILURE;
        goto cleanup_4;
    }

    SDL_PropertiesID props = SDL_GetRendererProperties(renderer);

    auto renderer_name =
        SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, "unknown");
    log_info("Using renderer \"%s\"", renderer_name);

    GameInfo info = gb_cartridge_info(rom);
    log_info("Cartridge type: $%02X", info.cart_type);
    log_info("RAM size: $%02X", info.ram_size);
    log_info("ROM size: $%02X", info.rom_size);
    log_info("Game title: %s", info.title);

    State state = state_init(&gb, window);

    SDL_RenderPresent(renderer);
    SDL_SetWindowResizable(window, true);
    run_until_quit(&state, renderer);

    SDL_DestroyRenderer(renderer);
cleanup_4:
    SDL_DestroyWindow(window);
cleanup_3:
    gb_deinit(&gb);
cleanup_2:
    free(boot_rom);
cleanup_1:
    free(rom);

    return retval;
}

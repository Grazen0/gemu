#include "cart.h"
#include "frontend/common.h"
#include "game_boy.h"
#include "log.h"
#include "scheduler.h"
#include "string.h"
#include "util.h"
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define EXE_NAME "gemu"

static constexpr char USAGE[] =
    "Usage: " EXE_NAME " [options] [rom_file]                                \n"
    "A Game Boy emulator written in C.                                       \n"
    "                                                                        \n"
    "  -h, --help                 show this help message                     \n"
    "  -b, --boot-rom FILE        boot ROM file to use                       \n"
    "  -l, --log-level LOG_LEVEL  log level (error, warn, info, debug, trace)\n";

static const struct option OPTIONS[] = {
    {     "help",       no_argument, nullptr, 'h'},
    { "boot-rom", required_argument, nullptr, 'b'},
    {"log-level", required_argument, nullptr, 'l'},
};

typedef struct {
    bool help;
    const char *rom_file;
    const char *boot_rom_file;
    LogLevel log_level;
} Args;

static Args args_init()
{
    return (Args){
        .help = false,
        .rom_file = nullptr,
        .boot_rom_file = nullptr,
        .log_level = LOG_LEVEL_INFO,
    };
}

static bool parse_args(int argc, char *argv[static 1], Args out_args[static 1])
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
                out_args->boot_rom_file = optarg;
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

    out_args->rom_file = argv[optind];
    return true;
}

static u8 *load_file(const char filename[static 1], size_t *data_size)
{
    FILE *file = fopen(filename, "r");
    if (file == nullptr)
        return nullptr;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    u8 *data = calloc(size, sizeof(*data));
    if (data == nullptr)
        goto cleanup;

    size_t read = fread(data, sizeof(*data), size, file);
    if (read != size)
        goto cleanup;

    if (data_size != nullptr)
        *data_size = size;

cleanup:
    fclose(file);
    return data;
}

static u8 *load_boot_rom(const char filename[static 1])
{

    size_t boot_rom_len = 0;
    u8 *boot_rom = load_file(filename, &boot_rom_len);

    if (boot_rom == nullptr) {
        log_error("Could not read boot ROM file: %s", strerror(errno));
        return nullptr;
    }

    if (boot_rom_len != GB_BOOT_ROM_LEN) {
        log_error("Boot ROM must be exactly %zu bytes long (was %zu)",
                  GB_BOOT_ROM_LEN, boot_rom_len);
        free(boot_rom);
        return nullptr;
    }

    return boot_rom;
}

static RingSink ring_sink;

static void crash_handler([[maybe_unused]] int sig)
{
    static constexpr char MESSAGE[] =
        "\n"
        "================ Last executed instructions ================\n";

    write(STDERR_FILENO, MESSAGE, sizeof(MESSAGE) - 1);
    ring_sink_dump(&ring_sink, STDERR_FILENO); // NOLINT
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
    u8 *rom = load_file(args.rom_file, &rom_len);

    if (rom == nullptr) {
        log_error("Could not read ROM file: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    CartInfo info = cart_info_from_rom(rom);
    log_info("Cartridge type: $%02X", info.cart_type);
    log_info("RAM size: $%02X", info.ram_size);
    log_info("ROM size: $%02X", info.rom_size);
    log_info("Game title: %s", info.title);

    int retval = EXIT_SUCCESS;
    u8 *boot_rom = nullptr;

    if (args.boot_rom_file != nullptr) {
        boot_rom = load_boot_rom(args.boot_rom_file);

        if (boot_rom == nullptr) {
            log_error(
                "Boot ROM file either does not exist or has invalid size.");
            retval = EXIT_FAILURE;
            goto cleanup;
        }
    }

    size_t trace_depth = 32;

    const char *trace_depth_str = getenv("GEMU_TRACE_DEPTH");
    if (trace_depth_str != nullptr)
        trace_depth = strtoll(trace_depth_str, nullptr, 10);

    ring_sink = ring_sink_init(trace_depth, 256);

    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGFPE, crash_handler);
    signal(SIGILL, crash_handler);
    signal(SIGBUS, crash_handler);

    GameBoy gb = gb_init(ring_sink_as_sink(&ring_sink), boot_rom);
    gb_load_rom(&gb, rom_len, rom);

    Scheduler sched = sched_init();

    log_info("Using frontend \"%s\"", selected_frontend.name);
    selected_frontend.run(&gb, &sched);

    log_info("Cleaning up other allocations");

    ring_sink_deinit(&ring_sink);
    sched_deinit(&sched);
    gb_deinit(&gb);
    free(boot_rom);
cleanup:
    free(rom);

    return retval;
}

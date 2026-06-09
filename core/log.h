#ifndef GEMU_LOG_H
#define GEMU_LOG_H

#include "util.h"
#include <stdarg.h>
#include <stddef.h>

typedef enum : u8 {
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE,

    LOG_LEVEL_COUNT,
} LogLevel;

[[nodiscard]] bool log_level_from_str(const char *str, LogLevel *out);

void logger_set_level(LogLevel log_level);

[[gnu::format(printf, 1, 2)]] void log_trace(const char *format, ...);

[[gnu::format(printf, 1, 2)]] void log_debug(const char *format, ...);

[[gnu::format(printf, 1, 2)]] void log_info(const char *format, ...);

[[gnu::format(printf, 1, 2)]] void log_warn(const char *format, ...);

[[gnu::format(printf, 1, 2)]] void log_error(const char *format, ...);

#endif

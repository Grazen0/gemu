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

/**
 * \brief Converts a human-readable log level string into a LogLevel variant.
 *
 * For example, "debug" is converted to LogLevel_Debug.
 *
 * \param str a non-null string to convert into a LogLevel variant.
 * \param out the place to store the result at.
 *
 * \return whether the conversion was successful or not.
 *
 * \sa LogLevel
 */
[[nodiscard]] bool log_level_from_str(const char *str, LogLevel *out);

/**
 * \brief Initializes logging.
 *
 * This function must be called before using any other log-related functions.
 *
 * \param log_level LogLevel to use.
 *
 * \sa logger_cleanup
 */
void logger_set_level(LogLevel log_level);

[[gnu::format(printf, 1, 2)]] void log_trace(const char *format, ...);

[[gnu::format(printf, 1, 2)]] void log_debug(const char *format, ...);

[[gnu::format(printf, 1, 2)]] void log_info(const char *format, ...);

[[gnu::format(printf, 1, 2)]] void log_warn(const char *format, ...);

[[gnu::format(printf, 1, 2)]] void log_error(const char *format, ...);

#endif

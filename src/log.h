#ifndef GEMU_LOG_H
#define GEMU_LOG_H

#include "stdinc.h"
#include <SDL3/SDL.h>
#include <stdarg.h>
#include <stddef.h>

typedef enum : u8 {
    LogLevel_Trace = 4,
    LogLevel_Debug = 3,
    LogLevel_Info = 2,
    LogLevel_Warn = 1,
    LogLevel_Error = 0,
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
bool LogLevel_from_str(const char *str, LogLevel *out);

/**
 * \brief Initializes logging.
 *
 * This function must be called before using any other log-related functions.
 * Otherwise, no logging will take effect.
 *
 * \param log_level LogLevel to use.
 *
 * \sa logger_cleanup
 */
void logger_init(LogLevel log_level);

/**
 * \brief Cleans up logging resources.
 *
 * This function must be called after logger_init.
 *
 * \sa logger_init
 */
void logger_cleanup();

void log_trace(const char *format, ...);

void log_debug(const char *format, ...);

void log_info(const char *format, ...);

void log_warn(const char *format, ...);

void log_error(const char *format, ...);

#endif

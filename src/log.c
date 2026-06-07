#include "log.h"
#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    LogLevel level;
} LoggerContext;

static const char *LABELS[] = {
    [LOG_LEVEL_TRACE] = "\033[90mTRACE", [LOG_LEVEL_DEBUG] = "\033[36mDEBUG",
    [LOG_LEVEL_INFO] = "\033[34mINFO",   [LOG_LEVEL_WARN] = "\033[33mWARN",
    [LOG_LEVEL_ERROR] = "\033[31mERROR",
};

static constexpr size_t LABELS_LEN = ARRAY_LEN(LABELS);
static_assert(LABELS_LEN == LOG_LEVEL_COUNT);

static const char *log_level_label(LogLevel level)
{

    if (level >= LABELS_LEN)
        unreachable();

    return LABELS[level];
}

static void log_ctx_vlog(LoggerContext *ctx, LogLevel level, const char *format,
                         va_list args)
{
    if (level > ctx->level)
        return;

    const char *label = log_level_label(level);
    FILE *stream = level == LOG_LEVEL_ERROR ? stderr : stdout;

    fprintf(stream, "\033[90m[%s\033[90m]:\033[0m ", label);
    vfprintf(stream, format, args);
    fputc('\n', stream);
}

bool log_level_from_str(const char *str, LogLevel *out)
{
    static const struct {
        const char *name;
        LogLevel level;
    } ALTERNATIVES[] = {
        {"trace", LOG_LEVEL_TRACE},
        {"debug", LOG_LEVEL_DEBUG},
        { "info",  LOG_LEVEL_INFO},
        { "warn",  LOG_LEVEL_WARN},
        {"error", LOG_LEVEL_ERROR},
    };

    static constexpr size_t ALTERNATIVES_LEN = ARRAY_LEN(ALTERNATIVES);

    for (size_t i = 0; i < ALTERNATIVES_LEN; ++i) {
        if (strcmp(str, ALTERNATIVES[i].name) == 0) {
            *out = ALTERNATIVES[i].level;
            return true;
        }
    }

    return false;
}

static LoggerContext global_ctx = {
    .level = LOG_LEVEL_INFO,
};

static void vlog(LogLevel level, const char *format, va_list args)
{
    log_ctx_vlog(&global_ctx, level, format, args);
}

void logger_set_level(LogLevel level)
{
    global_ctx.level = level;
}

void log_trace(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_LEVEL_TRACE, format, args);
    va_end(args);
}

void log_debug(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

void log_info(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_LEVEL_INFO, format, args);
    va_end(args);
}

void log_warn(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_LEVEL_WARN, format, args);
    va_end(args);
}

void log_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_LEVEL_ERROR, format, args);
    va_end(args);
}

#include "log.h"
#include "util.h"
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
    [LOG_LEVEL_TRACE] = "TRACE", [LOG_LEVEL_DEBUG] = "DEBUG",
    [LOG_LEVEL_INFO] = "INFO",   [LOG_LEVEL_WARN] = "WARN",
    [LOG_LEVEL_ERROR] = "ERROR",
};

static constexpr size_t LABELS_LEN = ARRAY_LEN(LABELS);
static_assert(LABELS_LEN == LOG_LEVEL_COUNT);

static const char *log_level_label(LogLevel level)
{

    if (level >= LABELS_LEN)
        unreachable();

    return LABELS[level];
}

static void log_ctx_vlog(LoggerContext ctx[static 1], LogLevel level, const char format[static 1],
                         va_list args)
{
    if (level > ctx->level)
        return;

    const char *label = log_level_label(level);
    FILE *stream = level == LOG_LEVEL_ERROR ? stderr : stdout;

    fprintf(stream, "[%s] ", label);
    vfprintf(stream, format, args);
    fputc('\n', stream);
}

bool log_level_from_str(const char str[static 1], LogLevel out[static 1])
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

static void vlog(LogLevel level, const char format[static 1], va_list args)
{
    log_ctx_vlog(&global_ctx, level, format, args);
}

void logger_set_level(LogLevel level)
{
    global_ctx.level = level;
}

void log_trace(const char format[static 1], ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_LEVEL_TRACE, format, args);
    va_end(args);
}

void log_debug(const char format[static 1], ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

void log_info(const char format[static 1], ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_LEVEL_INFO, format, args);
    va_end(args);
}

void log_warn(const char format[static 1], ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_LEVEL_WARN, format, args);
    va_end(args);
}

void log_error(const char format[static 1], ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_LEVEL_ERROR, format, args);
    va_end(args);
}

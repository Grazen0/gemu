#include "log.h"
#include "control.h"
#include <SDL3/SDL_mutex.h>
#include <SDL3/SDL_thread.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool logger_ready = false;
static LogLevel active_log_level = LogLevel_Info;
static SDL_Thread *logger_thread = nullptr;
static LogQueue log_queue = {};

static const char *log_level_label(const LogLevel level)
{
    switch (level) {
    case LogLevel_Trace:
        return "\033[90mTRACE";
    case LogLevel_Debug:
        return "\033[36mDEBUG";
    case LogLevel_Info:
        return "\033[34mINFO";
    case LogLevel_Warn:
        return "\033[33mWARN";
    case LogLevel_Error:
        return "\033[31mERROR";
    default:
        BAIL("unreachable");
    }
}

static void print_log_message(const LogMessage *const restrict message)
{
    const char *const label = log_level_label(message->level);
    FILE *const stream = message->level == LogLevel_Error ? stderr : stdout;

    fprintf(stream, "\033[90m[%s\033[90m]:\033[0m ", label);
    fputs(message->text, stream);
    fputc('\n', stream);
}

static int logger_thread_fn([[maybe_unused]] void *data)
{
    while (true) {
        SDL_LockMutex(log_queue.mtx);

        while (!log_queue.quit && log_queue.head == log_queue.tail)
            SDL_WaitCondition(log_queue.cond, log_queue.mtx);

        if (log_queue.quit && log_queue.head == log_queue.tail) {
            SDL_UnlockMutex(log_queue.mtx);
            break;
        }

        const LogMessage message = log_queue.messages[log_queue.tail];
        log_queue.tail = (log_queue.tail + 1) % log_queue.capacity;
        SDL_UnlockMutex(log_queue.mtx);

        print_log_message(&message);
    }

    return 0;
}

void logger_init(const LogLevel log_level)
{
    active_log_level = log_level;

    log_queue = (LogQueue){
        .capacity = 1,
        .messages = malloc(sizeof(log_queue.messages[0])),
        .head = 0,
        .tail = 0,
        .mtx = SDL_CreateMutex(),
        .cond = SDL_CreateCondition(),
        .quit = false,
    };

    // NOLINTNEXTLINE
    logger_thread = SDL_CreateThread(logger_thread_fn, "Logger", nullptr);

    atexit(logger_cleanup);
    logger_ready = true;
}

void logger_cleanup(void)
{
    logger_ready = false;

    SDL_LockMutex(log_queue.mtx);
    log_queue.quit = true;

    SDL_SignalCondition(log_queue.cond);
    SDL_UnlockMutex(log_queue.mtx);

    SDL_WaitThread(logger_thread, nullptr);
    logger_thread = nullptr;

    SDL_DestroyMutex(log_queue.mtx);
    log_queue.mtx = nullptr;

    SDL_DestroyCondition(log_queue.cond);
    log_queue.cond = nullptr;
}

static void grow_log_queue(void)
{
    const size_t new_capacity = log_queue.capacity * 2;

    LogMessage *const new_messages =
        malloc(new_capacity * sizeof(log_queue.messages[0]));

    if (new_messages == nullptr)
        BAIL("Could not reallocate space for log queue. errno %i", errno);

    const size_t old_size =
        ((log_queue.head - log_queue.tail) + log_queue.capacity) %
        log_queue.capacity;

    for (size_t i = 0; i < old_size; ++i) {
        size_t old_index = (log_queue.tail + i) % log_queue.capacity;
        new_messages[i] = log_queue.messages[old_index];
    }

    log_queue.tail = 0;
    log_queue.head = old_size;
    log_queue.capacity = new_capacity;

    free(log_queue.messages);
    log_queue.messages = new_messages;
}

static void vlog(const LogLevel level, const char *const restrict format,
                 va_list args)
{
    if (!logger_ready || level > active_log_level)
        return;

    SDL_LockMutex(log_queue.mtx);
    if ((log_queue.head + 1) % log_queue.capacity == log_queue.tail) {
        grow_log_queue();
    }

    LogMessage *const next_message = &log_queue.messages[log_queue.head];
    *next_message = (LogMessage){
        .text = {},
        .level = level,
    };
    vsnprintf(next_message->text, sizeof(next_message->text), format, args);

    log_queue.head = (log_queue.head + 1) % log_queue.capacity;
    SDL_SignalCondition(log_queue.cond);
    SDL_UnlockMutex(log_queue.mtx);
}

void log_trace(const char *const restrict format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LogLevel_Trace, format, args);
    va_end(args);
}

void log_debug(const char *const restrict format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LogLevel_Debug, format, args);
    va_end(args);
}

void log_info(const char *const restrict format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LogLevel_Info, format, args);
    va_end(args);
}

void log_warn(const char *const restrict format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LogLevel_Warn, format, args);
    va_end(args);
}

void log_error(const char *const restrict format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LogLevel_Error, format, args);
    va_end(args);
}

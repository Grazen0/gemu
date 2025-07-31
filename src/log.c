#include "log.h"
#include "control.h"
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool logger_ready = false;
static int current_category_mask = LogCategory_All;
static pthread_t logger_thread = 0;
static LogQueue log_queue = {};

static const char* level_label(const LogLevel level) {
    switch (level) {
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

void pretty_log(const LogLevel level, const LogCategory category, const char* const restrict text) {
    const char* const restrict label = level_label(level);
    FILE* const restrict stream = level == LogLevel_Error ? stderr : stdout;

    fprintf(stream, "\033[90m[%s\033[90m] (%d):\033[0m ", label, category);
    fputs(text, stream);
    fputc('\n', stream);
}

static void* logger_thread_fn([[maybe_unused]] void* _arg) {
    while (true) {
        pthread_mutex_lock(&log_queue.mtx);

        while (!log_queue.quit && log_queue.head == log_queue.tail) {
            pthread_cond_wait(&log_queue.cond, &log_queue.mtx);
        }

        if (log_queue.quit && log_queue.head == log_queue.tail) {
            pthread_mutex_unlock(&log_queue.mtx);
            break;
        }

        const LogMessage message = log_queue.messages[log_queue.tail];
        log_queue.tail = (log_queue.tail + 1) % log_queue.capacity;
        pthread_mutex_unlock(&log_queue.mtx);

        pretty_log(message.level, message.category, message.text);
    }

    return nullptr;
}

void logger_init(const int category_mask) {
    current_category_mask = category_mask;

    log_queue = (LogQueue){
        .capacity = 1,
        .messages = calloc(1, sizeof(log_queue.messages[0])),
        .head = 0,
        .tail = 0,
        .mtx = {},
        .cond = {},
        .quit = false,
    };

    pthread_mutex_init(&log_queue.mtx, nullptr);
    pthread_cond_init(&log_queue.cond, nullptr);

    pthread_create(&logger_thread, nullptr, logger_thread_fn, nullptr);

    atexit(logger_cleanup);
    logger_ready = true;
}

void logger_cleanup(void) {
    logger_ready = false;
    pthread_mutex_lock(&log_queue.mtx);
    log_queue.quit = true;
    pthread_cond_signal(&log_queue.cond);
    pthread_mutex_unlock(&log_queue.mtx);
    pthread_join(logger_thread, nullptr);

    pthread_mutex_destroy(&log_queue.mtx);
    pthread_cond_destroy(&log_queue.cond);
}

static void grow_log_queue(void) {
    const size_t new_capacity = log_queue.capacity * 2;

    LogMessage* const restrict new_messages = calloc(new_capacity, sizeof(log_queue.messages[0]));
    if (new_messages == nullptr) {
        BAIL("Could not reallocate space for log queue. errno %i", errno);
    }

    const size_t old_size =
        ((log_queue.head - log_queue.tail) + log_queue.capacity) % log_queue.capacity;

    // Copy logs to new messages. Reorders them as well.
    for (size_t i = 0; i < old_size; ++i) {
        size_t old_index = (log_queue.tail + i) % log_queue.capacity;
        memcpy(&new_messages[i], &log_queue.messages[old_index], sizeof(log_queue.messages[0]));
    }

    log_queue.tail = 0;
    log_queue.head = old_size;
    log_queue.capacity = new_capacity;
    free(log_queue.messages);
    log_queue.messages = new_messages;
}

void vlog(
    const LogLevel level, const LogCategory category, const char* const restrict format,
    va_list args
) {
    if (!logger_ready)
        return;

    pthread_mutex_lock(&log_queue.mtx);
    if ((log_queue.head + 1) % log_queue.capacity == log_queue.tail) {
        grow_log_queue();
    }

    LogMessage* const next_message = &log_queue.messages[log_queue.head];
    *next_message = (LogMessage){
        .text = {},
        .level = level,
        .category = category,
    };
    vsnprintf(next_message->text, sizeof(next_message->text), format, args);

    log_queue.head = (log_queue.head + 1) % log_queue.capacity;
    pthread_cond_signal(&log_queue.cond);
    pthread_mutex_unlock(&log_queue.mtx);
}

void log_info(const LogCategory category, const char* const restrict format, ...) {
    if ((current_category_mask & category) != 0) {
        va_list args;
        va_start(args, format);
        vlog(LogLevel_Info, category, format, args);
        va_end(args);
    }
}

void log_warn(const LogCategory category, const char* const restrict format, ...) {
    if ((current_category_mask & category) != 0) {
        va_list args;
        va_start(args, format);
        vlog(LogLevel_Warn, category, format, args);
        va_end(args);
    }
}

void log_error(const LogCategory category, const char* const restrict format, ...) {
    if ((current_category_mask & category) != 0) {
        va_list args;
        va_start(args, format);
        vlog(LogLevel_Error, category, format, args);
        va_end(args);
    }
}

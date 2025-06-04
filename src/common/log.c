#include "common/log.h"
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/control.h"

static LogFn current_log_fn = NULL;
static int current_category_mask = LogCategory_ALL;
static pthread_t logger_thread = 0;
static LogQueue log_queue = {0};

static void
log_fallback(const LogLevel level, const LogCategory category, const char* const restrict text) {
    FILE* const restrict stream = level == LogLevel_ERROR ? stderr : stdout;
    fprintf(stream, "(%d) ", category);
    fputs(text, stream);
    fputc('\n', stream);
    fflush(stream);
}

static void* logger_thread_fn(void* const restrict _arg) {
    (void)_arg;  // Tell the compiler to stfu

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

        const LogFn log_fn = current_log_fn != NULL ? current_log_fn : log_fallback;
        log_fn(message.level, message.category, message.text);
    }

    return NULL;
}

void logger_set_category_mask(const int category_mask) {
    current_category_mask = category_mask;
}

int logger_init(const LogFn log_fn) {
    current_log_fn = log_fn;

    log_queue = (LogQueue){
        .capacity = 1,
        .messages = calloc(1, sizeof(log_queue.messages[0])),
        .head = 0,
        .tail = 0,
        .mtx = PTHREAD_MUTEX_INITIALIZER,
        .cond = PTHREAD_COND_INITIALIZER,
        .quit = false,
    };

    pthread_mutex_init(&log_queue.mtx, NULL);
    pthread_cond_init(&log_queue.cond, NULL);

    const int result = pthread_create(&logger_thread, NULL, logger_thread_fn, NULL);
    if (result != 0) {
        return result;
    }

    atexit(logger_cleanup);
    return 0;
}

void logger_cleanup(void) {
    pthread_mutex_lock(&log_queue.mtx);
    log_queue.quit = true;
    pthread_cond_signal(&log_queue.cond);
    pthread_mutex_unlock(&log_queue.mtx);
    pthread_join(logger_thread, NULL);
}

static void grow_log_queue(void) {
    const size_t new_capacity = log_queue.capacity * 2;

    LogMessage* const restrict new_messages = calloc(new_capacity, sizeof(log_queue.messages[0]));
    if (new_messages == NULL) {
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
    const LogLevel level,
    const LogCategory category,
    const char* const restrict format,
    va_list args
) {
    pthread_mutex_lock(&log_queue.mtx);
    if ((log_queue.head + 1) % log_queue.capacity == log_queue.tail) {
        grow_log_queue();
    }

    LogMessage* const restrict next_message = &log_queue.messages[log_queue.head];
    *next_message = (LogMessage){
        .text = {0},
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
        vlog(LogLevel_INFO, category, format, args);
        va_end(args);
    }
}

void log_warn(const LogCategory category, const char* const restrict format, ...) {
    if ((current_category_mask & category) != 0) {
        va_list args;
        va_start(args, format);
        vlog(LogLevel_WARN, category, format, args);
        va_end(args);
    }
}

void log_error(const LogCategory category, const char* const restrict format, ...) {
    if ((current_category_mask & category) != 0) {
        va_list args;
        va_start(args, format);
        vlog(LogLevel_ERROR, category, format, args);
        va_end(args);
    }
}

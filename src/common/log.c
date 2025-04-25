#include "common/log.h"
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const Logger* current_logger = NULL;

static void ensure_trailing_newline(const char* const restrict stream, FILE* const restrict file) {
    const size_t len = strlen(stream);
    if (len == 0 || stream[len - 1] != '\n') {
        fputc('\n', file);
    }
}

void set_logger(const Logger* const restrict logger) {
    current_logger = logger;
}

void log_info(const char* const restrict format, ...) {
    va_list args;
    va_start(args, format);

    if (current_logger != NULL && current_logger->info != NULL) {
        current_logger->info(format, args);
    } else {
        vprintf(format, args);
        ensure_trailing_newline(format, stdout);
    }

    va_end(args);
}

void log_warn(const char* const restrict format, ...) {
    va_list args;
    va_start(args, format);

    if (current_logger != NULL && current_logger->warn != NULL) {
        current_logger->warn(format, args);
    } else {
        vprintf(format, args);
        ensure_trailing_newline(format, stdout);
    }

    va_end(args);
}

void log_error(const char* const restrict format, ...) {
    va_list args;
    va_start(args, format);

    if (current_logger != NULL && current_logger->error != NULL) {
        current_logger->error(format, args);
    } else {
        vfprintf(stderr, format, args);
        ensure_trailing_newline(format, stderr);
    }

    va_end(args);
}

void vlog_info(const char* const restrict format, va_list args) {
    if (current_logger != NULL && current_logger->info != NULL) {
        current_logger->info(format, args);
    } else {
        vprintf(format, args);
        ensure_trailing_newline(format, stdout);
    }
}

void vlog_warn(const char* const restrict format, va_list args) {
    if (current_logger != NULL && current_logger->warn != NULL) {
        current_logger->warn(format, args);
    } else {
        vprintf(format, args);
        ensure_trailing_newline(format, stdout);
    }
}

void vlog_error(const char* const restrict format, va_list args) {
    if (current_logger != NULL && current_logger->error != NULL) {
        current_logger->error(format, args);
    } else {
        vfprintf(stderr, format, args);
        ensure_trailing_newline(format, stderr);
    }
}

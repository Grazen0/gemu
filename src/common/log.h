#ifndef LIB_LOG_H
#define LIB_LOG_H

#include <stdarg.h>
#include <stdbool.h>

typedef struct Logger {
    void (*info)(const char* format, va_list args);
    void (*warn)(const char* format, va_list args);
    void (*error)(const char* format, va_list args);
} Logger;

void set_logger(const Logger* restrict new_logger);

void log_info(const char* restrict format, ...);

void log_warn(const char* restrict format, ...);

void log_error(const char* restrict format, ...);

void vlog_info(const char* restrict format, va_list args);

void vlog_warn(const char* restrict format, va_list args);

void vlog_error(const char* restrict format, va_list args);

#endif

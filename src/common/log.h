#ifndef LIB_LOG_H
#define LIB_LOG_H

#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

typedef enum LogLevel {
    LogLevel_INFO,
    LogLevel_WARN,
    LogLevel_ERROR,
} LogLevel;

typedef enum LogCategory {
    LogCategory_ALL = ~0,
    LogCategory_NONE = 0,
    LogCategory_INSTRUCTION = 1 << 0,
    LogCategory_ERROR = 1 << 1,
    LogCategory_IO = 1 << 2,
    LogCategory_MEMORY = 1 << 3,
    LogCategory_INTERRUPT = 1 << 4,
    LogCategory_KEEP = 1 << 5,
    LogCategory_TODO = 1 << 6,
} LogCategory;

typedef void (*LogFn)(LogLevel level, LogCategory category, const char* restrict text);

typedef struct LogMessage {
    LogCategory category;
    LogLevel level;
    char text[256];
} LogMessage;

typedef struct LogQueue {
    size_t capacity;
    LogMessage* messages;
    size_t head;
    size_t tail;
    pthread_mutex_t mtx;
    pthread_cond_t cond;
    bool quit;
} LogQueue;

int logger_init(LogFn log_fn, int category_mask);

void logger_cleanup(void);

void vlog(LogLevel level, LogCategory category, const char* restrict format, va_list args);

void log_info(LogCategory category, const char* restrict format, ...);

void log_warn(LogCategory category, const char* restrict format, ...);

void log_error(LogCategory category, const char* restrict format, ...);

#endif

#ifndef GEMU_LOG_H
#define GEMU_LOG_H

#include "stdinc.h"
#include <SDL3/SDL_mutex.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

static constexpr size_t LOG_MESSAGE_CAPACITY = 256;

typedef enum : u8 {
    LogLevel_Trace = 4,
    LogLevel_Debug = 3,
    LogLevel_Info = 2,
    LogLevel_Warn = 1,
    LogLevel_Error = 0,
} LogLevel;

typedef struct {
    LogLevel level;
    char text[LOG_MESSAGE_CAPACITY];
} LogMessage;

typedef struct {
    size_t capacity;
    LogMessage *messages;
    size_t head;
    size_t tail;
    SDL_Mutex *mtx;
    SDL_Condition *cond;
    bool quit;
} LogQueue;

LogLevel LogLevel_from_str(const char * str);

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
void logger_cleanup(void);

void log_trace(const char * format, ...);

void log_debug(const char * format, ...);

void log_info(const char * format, ...);

void log_warn(const char * format, ...);

void log_error(const char * format, ...);

#endif

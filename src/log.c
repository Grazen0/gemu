#include "log.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static constexpr size_t LOG_MESSAGE_CAPACITY = 256;

typedef struct {
    LogLevel level;
    char text[LOG_MESSAGE_CAPACITY];
} LogMessage;

typedef struct {
    LogMessage *items;
    size_t capacity;
    size_t head;
    size_t tail;
} LogQueue;

typedef struct {
    LogLevel level;
    SDL_Thread *thread;
    LogQueue queue;
    SDL_Mutex *queue_mtx;
    SDL_Condition *cond;
    bool quit;
} LoggerContext;

static LogQueue queue_init()
{
    return (LogQueue){
        .items = nullptr,
        .capacity = 0,
        .head = 0,
        .tail = 0,
    };
}

static void queue_deinit(LogQueue *queue)
{
    free(queue->items);
    *queue = queue_init();
}

static void queue_grow(LogQueue *queue)
{
    static constexpr size_t INIT_CAPACITY = 32;

    if (queue->capacity == 0) {
        queue->items = calloc(INIT_CAPACITY, sizeof(queue->items[0]));
        assert(queue->items != nullptr);
        queue->capacity = INIT_CAPACITY;
        return;
    }

    size_t new_capacity = queue->capacity * 2;

    LogMessage *new_messages = calloc(new_capacity, sizeof(queue->items[0]));
    assert(new_messages != nullptr);

    size_t old_size =
        ((queue->head - queue->tail) + queue->capacity) % queue->capacity;

    for (size_t i = 0; i < old_size; ++i) {
        size_t old_index = (queue->tail + i) % queue->capacity;
        new_messages[i] = queue->items[old_index];
    }

    queue->tail = 0;
    queue->head = old_size;
    queue->capacity = new_capacity;

    free(queue->items);
    queue->items = new_messages;
}

static bool queue_is_full(const LogQueue *queue)
{
    return queue->capacity == 0 ||
           (queue->head + 1) % queue->capacity == queue->tail;
}

static bool queue_is_empty(const LogQueue *queue)
{
    return queue->head == queue->tail;
}

static void queue_enqueue(LogQueue *queue, LogMessage message)
{
    if (queue_is_full(queue))
        queue_grow(queue);

    assert(!queue_is_full(queue));
    queue->items[queue->head] = message;
    queue->head = (queue->head + 1) % queue->capacity;
}

static LogMessage queue_dequeue(LogQueue *queue)
{
    assert(!queue_is_empty(queue));

    LogMessage out = queue->items[queue->tail];
    queue->tail = (queue->tail + 1) % queue->capacity;
    return out;
}

static const char *log_level_label(LogLevel level)
{
    static const char *LABELS[] = {
        [LOG_TRACE] = "\033[90mTRACE", [LOG_DEBUG] = "\033[36mDEBUG",
        [LOG_INFO] = "\033[34mINFO",   [LOG_WARN] = "\033[33mWARN",
        [LOG_ERROR] = "\033[31mERROR",
    };

    static constexpr size_t LABELS_LEN = sizeof(LABELS) / sizeof(LABELS[0]);
    static_assert(LABELS_LEN == LOG_LEVEL_COUNT);

    if (level >= LABELS_LEN)
        unreachable();

    return LABELS[level];
}

static void print_log_message(const LogMessage *message)
{
    const char *label = log_level_label(message->level);
    FILE *stream = message->level == LOG_ERROR ? stderr : stdout;

    fprintf(stream, "\033[90m[%s\033[90m]:\033[0m ", label);
    fputs(message->text, stream);
    fputc('\n', stream);
}

static int log_thread_fn(void *data)
{
    LoggerContext *ctx = data;

    while (true) {
        SDL_LockMutex(ctx->queue_mtx);

        while (!ctx->quit && queue_is_empty(&ctx->queue))
            SDL_WaitCondition(ctx->cond, ctx->queue_mtx);

        if (ctx->quit && queue_is_empty(&ctx->queue)) {
            SDL_UnlockMutex(ctx->queue_mtx);
            break;
        }

        LogMessage message = queue_dequeue(&ctx->queue);
        SDL_UnlockMutex(ctx->queue_mtx);

        print_log_message(&message);
    }

    return 0;
}

static LoggerContext log_ctx_init(LogLevel level)
{
    return (LoggerContext){
        .level = level,
        .thread = nullptr,
        .queue = queue_init(),
        .queue_mtx = SDL_CreateMutex(),
        .cond = SDL_CreateCondition(),
        .quit = false,
    };
}

static void log_ctx_deinit(LoggerContext *ctx)
{
    if (ctx != nullptr) {
        queue_deinit(&ctx->queue);

        SDL_DestroyMutex(ctx->queue_mtx);
        ctx->queue_mtx = nullptr;

        SDL_DestroyCondition(ctx->cond);
        ctx->cond = nullptr;
    }
}

static void log_ctx_spawn_thread(LoggerContext *ctx)
{
    assert(ctx->thread == nullptr);
    ctx->thread = SDL_CreateThread(log_thread_fn, "Logger", ctx);
}

static void log_ctx_clean_thread(LoggerContext *ctx)
{
    SDL_LockMutex(ctx->queue_mtx);
    ctx->quit = true;
    SDL_SignalCondition(ctx->cond);
    SDL_UnlockMutex(ctx->queue_mtx);

    SDL_WaitThread(ctx->thread, nullptr);
    ctx->thread = nullptr;
}

static void log_ctx_vlog(LoggerContext *ctx, LogLevel level, const char *format,
                         va_list args)
{
    if (level > ctx->level)
        return;

    LogMessage message = {.text = {}, .level = level};
    vsnprintf(message.text, sizeof(message.text), format, args);

    SDL_LockMutex(ctx->queue_mtx);
    queue_enqueue(&ctx->queue, message);
    SDL_SignalCondition(ctx->cond);
    SDL_UnlockMutex(ctx->queue_mtx);
}

bool log_level_from_str(const char *str, LogLevel *out)
{
    if (strcmp(str, "trace") == 0)
        *out = LOG_TRACE;
    else if (strcmp(str, "debug") == 0)
        *out = LOG_DEBUG;
    else if (strcmp(str, "info") == 0)
        *out = LOG_INFO;
    else if (strcmp(str, "warn") == 0)
        *out = LOG_WARN;
    else if (strcmp(str, "error") == 0)
        *out = LOG_ERROR;
    else
        return false;

    return true;
}

static LoggerContext global_ctx;
static bool global_ctx_inited = false;

void logger_init(LogLevel level)
{
    assert(!global_ctx_inited);

    global_ctx = log_ctx_init(level);
    global_ctx_inited = true;

    log_ctx_spawn_thread(&global_ctx);
    atexit(logger_cleanup);
}

void logger_cleanup()
{
    assert(global_ctx_inited);

    log_ctx_clean_thread(&global_ctx);

    log_ctx_deinit(&global_ctx);
    global_ctx_inited = false;
}

static void vlog(LogLevel level, const char *format, va_list args)
{
    assert(global_ctx_inited);
    log_ctx_vlog(&global_ctx, level, format, args);
}

void log_trace(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_TRACE, format, args);
    va_end(args);
}

void log_debug(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_DEBUG, format, args);
    va_end(args);
}

void log_info(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_INFO, format, args);
    va_end(args);
}

void log_warn(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_WARN, format, args);
    va_end(args);
}

void log_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vlog(LOG_ERROR, format, args);
    va_end(args);
}

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
    SDL_Mutex *quit_queue_mtx;
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
    bool quit = false;

    while (!quit) {
        SDL_LockMutex(ctx->quit_queue_mtx);

        while (!ctx->quit && queue_is_empty(&ctx->queue))
            SDL_WaitCondition(ctx->cond, ctx->quit_queue_mtx);

        while (!queue_is_empty(&ctx->queue)) {
            LogMessage message = queue_dequeue(&ctx->queue);
            print_log_message(&message);
        }

        quit = ctx->quit;
        SDL_UnlockMutex(ctx->quit_queue_mtx);
    }

    return 0;
}

static LoggerContext log_ctx_init()
{
    return (LoggerContext){
        .level = LOG_INFO,
        .thread = nullptr,
        .queue = queue_init(),
        .quit_queue_mtx = SDL_CreateMutex(),
        .cond = SDL_CreateCondition(),
        .quit = false,
    };
}

static bool log_ctx_is_started(const LoggerContext *ctx)
{
    return ctx->thread != nullptr;
}

static void log_ctx_stop(LoggerContext *ctx)
{
    if (!log_ctx_is_started(ctx))
        return;

    log_debug("Logger stopped.");

    SDL_LockMutex(ctx->quit_queue_mtx);
    ctx->quit = true;
    SDL_SignalCondition(ctx->cond);
    SDL_UnlockMutex(ctx->quit_queue_mtx);

    SDL_WaitThread(ctx->thread, nullptr);
    ctx->thread = nullptr;
}

static void log_ctx_deinit(LoggerContext *ctx)
{
    if (ctx == nullptr)
        return;

    log_ctx_stop(ctx);

    SDL_LockMutex(ctx->quit_queue_mtx);
    SDL_DestroyMutex(ctx->quit_queue_mtx);
    ctx->quit_queue_mtx = nullptr;

    SDL_DestroyCondition(ctx->cond);
    ctx->cond = nullptr;

    queue_deinit(&ctx->queue);
}

static void log_ctx_start(LoggerContext *ctx)
{
    if (log_ctx_is_started(ctx))
        return;

    ctx->thread = SDL_CreateThread(log_thread_fn, "logger", ctx);
}

static void log_ctx_vlog(LoggerContext *ctx, LogLevel level, const char *format,
                         va_list args)
{
    if (level > ctx->level)
        return;

    LogMessage message = {.text = {}, .level = level};
    vsnprintf(message.text, sizeof(message.text), format, args);

    SDL_LockMutex(ctx->quit_queue_mtx);
    queue_enqueue(&ctx->queue, message);
    SDL_SignalCondition(ctx->cond);
    SDL_UnlockMutex(ctx->quit_queue_mtx);
}

bool log_level_from_str(const char *str, LogLevel *out)
{
    static const struct {
        const char *name;
        LogLevel level;
    } ALTERNATIVES[] = {
        {"trace", LOG_TRACE},
        {"debug", LOG_DEBUG},
        { "info",  LOG_INFO},
        { "warn",  LOG_WARN},
        {"error", LOG_ERROR},
    };

    static constexpr size_t ALTERNATIVES_LEN =
        sizeof(ALTERNATIVES) / sizeof(ALTERNATIVES[0]);

    for (size_t i = 0; i < ALTERNATIVES_LEN; ++i) {
        if (strcmp(str, ALTERNATIVES[i].name) == 0) {
            *out = ALTERNATIVES[i].level;
            return true;
        }
    }

    return false;
}

static LoggerContext global_ctx;
static bool global_ctx_inited = false;

static void ensure_logger_inited()
{
    if (global_ctx_inited)
        return;

    global_ctx = log_ctx_init();
    global_ctx_inited = true;
}

static void logger_cleanup()
{
    if (!global_ctx_inited)
        return;

    log_ctx_stop(&global_ctx);
    log_ctx_deinit(&global_ctx);
    global_ctx_inited = false;
}

static void ensure_logger_started()
{
    ensure_logger_inited();

    if (!log_ctx_is_started(&global_ctx)) {
        log_ctx_start(&global_ctx);
        atexit(logger_cleanup);
    }
}

static void vlog(LogLevel level, const char *format, va_list args)
{
    ensure_logger_started();
    log_ctx_vlog(&global_ctx, level, format, args);
}

void logger_set_level(LogLevel level)
{
    ensure_logger_started();
    global_ctx.level = level;
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

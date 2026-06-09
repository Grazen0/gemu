#ifndef GEMU_SINK_H
#define GEMU_SINK_H

#include "util.h"
#include <stddef.h>
#include <stdio.h>

typedef struct {
    void (*vlog)(void *ptr, const char format[], va_list args);
    void (*deinit)(void *ptr);
} SinkVTable;

typedef struct {
    void *ptr;
    const SinkVTable *vtable;
} Sink;

static inline void sink_vlog(Sink sink, const char format[], va_list args)
{
    sink.vtable->vlog(sink.ptr, format, args);
}

static inline void sink_deinit(Sink sink)
{
    sink.vtable->deinit(sink.ptr);
}

void sink_box_deinit(Sink *sink);

[[gnu::format(printf, 2, 3)]] void sink_log(Sink sink, const char format[],
                                            ...);

static inline void void_sink_vlog_v([[maybe_unused]] void *ptr,
                                    [[maybe_unused]] const char format[],
                                    [[maybe_unused]] va_list args)
{
}

static inline void void_sink_deinit_v([[maybe_unused]] void *ptr)
{
}

typedef struct {
    char *buf;
    size_t buf_len;
    size_t max_msg_len;
    size_t head;
    size_t tail;
} RingSink;

RingSink ring_sink_init(size_t buf_size, size_t max_msg_size);

void ring_sink_deinit(RingSink *sink);

void ring_sink_vlog(RingSink *sink, const char format[], va_list args);

void ring_sink_dump(RingSink *sink, int fd);

DECL_UPCASTS(RingSink, ring_sink, Sink, sink)

extern const Sink void_sink;

#endif

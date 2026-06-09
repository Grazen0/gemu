#include "util.h"
#include "macros.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void sink_box_deinit(Sink *sink)
{
    sink_deinit(*sink);
    free(sink->ptr);
    sink->ptr = nullptr;
}

void sink_log(Sink sink, const char format[], ...)
{
    va_list args;
    va_start(args, format);
    sink_vlog(sink, format, args);
    va_end(args);
}

RingSink ring_sink_init(size_t buf_size, size_t max_msg_size)
{
    assert(buf_size > 0);
    assert(max_msg_size > 0);

    char *buf = calloc(buf_size, max_msg_size);
    assert(buf != nullptr);

    return (RingSink){
        .buf = buf,
        .buf_len = buf_size,
        .max_msg_len = max_msg_size,
        .head = 0,
        .tail = 0,
    };
}

void ring_sink_deinit(RingSink *sink)
{
    free(sink->buf);
    sink->buf = nullptr;
}

static char *ring_sink_slot(RingSink *sink, size_t idx)
{
    assert(idx < sink->buf_len);
    return &sink->buf[idx * sink->max_msg_len];
}

void ring_sink_vlog(RingSink *sink, const char format[], va_list args)
{
    char *message = ring_sink_slot(sink, sink->tail);
    vsnprintf(message, sink->max_msg_len, format, args);
    sink->tail = (sink->tail + 1) % sink->buf_len;

    if (sink->tail == sink->head)
        sink->head = (sink->head + 1) % sink->buf_len;
}

void ring_sink_dump(RingSink *sink, int fd)
{
    size_t cur = sink->head;

    size_t i = 1;

    while (cur != sink->tail) {
        char *message = ring_sink_slot(sink, cur);
        size_t len = strnlen(message, sink->max_msg_len);

        printf("%4zu: ", i);
        fflush(stdout);
        write(fd, message, len);
        write(fd, "\n", 1);

        cur = (cur + 1) % sink->buf_len;
        ++i;
    }
}

static void ring_sink_vlog_v(void *sink, const char format[], va_list args)
{
    ring_sink_vlog(sink, format, args);
}

static void ring_sink_deinit_v(void *sink)
{
    ring_sink_deinit(sink);
}

IMPL_UPCASTS(RingSink, ring_sink, Sink, sink, .vlog = ring_sink_vlog_v,
             .deinit = ring_sink_deinit_v)

const Sink void_sink = {
    .ptr = nullptr,
    .vtable = &void_sink_vtable,
};

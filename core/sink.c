#include "sink.h"
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

void sink_box_deinit(Sink sink[static 1])
{
    sink_deinit(*sink);
    free(sink->ptr);
    sink->ptr = nullptr;
}

void sink_log(Sink sink, const char format[static 1], ...)
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

void ring_sink_deinit(RingSink sink[static 1])
{
    free(sink->buf);
    sink->buf = nullptr;
}

static char *ring_sink_slot(RingSink sink[static 1], size_t idx)
{
    assert(idx < sink->buf_len);
    return &sink->buf[idx * sink->max_msg_len];
}

void ring_sink_vlog(RingSink sink[static 1], const char format[static 1], va_list args)
{
    char *message = ring_sink_slot(sink, sink->tail);
    vsnprintf(message, sink->max_msg_len, format, args);
    sink->tail = (sink->tail + 1) % sink->buf_len;

    if (sink->tail == sink->head)
        sink->head = (sink->head + 1) % sink->buf_len;
}

void ring_sink_dump(RingSink sink[static 1], int fd)
{
    size_t cur = sink->head;

    while (cur != sink->tail) {
        char *message = ring_sink_slot(sink, cur);
        size_t len = strnlen(message, sink->max_msg_len);

        write(fd, message, len);
        write(fd, "\n", 1);

        cur = (cur + 1) % sink->buf_len;
    }
}

static void ring_sink_vlog_v(void *sink, const char format[static 1], va_list args)
{
    ring_sink_vlog(sink, format, args);
}

static void ring_sink_deinit_v(void *sink)
{
    ring_sink_deinit(sink);
}

IMPL_UPCASTS(RingSink, ring_sink, Sink, sink, .vlog = ring_sink_vlog_v,
             .deinit = ring_sink_deinit_v)

static const SinkVTable void_sink_vtable = {
    .vlog = void_sink_vlog_v,
    .deinit = void_sink_deinit_v,
};

const Sink void_sink = {
    .ptr = nullptr,
    .vtable = &void_sink_vtable,
};

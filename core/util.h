#ifndef GEMU_UTIL_H
#define GEMU_UTIL_H

#include "macros.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

[[nodiscard]] static inline u16 concat_u16(u8 hi, u8 lo)
{
    return ((u16)hi << 8) | (u16)lo;
}

static inline void set_bits(u8 *dest, u8 mask, bool value)
{
    if (value)
        *dest |= mask;
    else
        *dest &= ~mask;
}

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

static const SinkVTable void_sink_vtable = {
    .vlog = void_sink_vlog_v,
    .deinit = void_sink_deinit_v,
};

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

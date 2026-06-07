#include "scheduler.h"
#include "game_boy.h"
#include "stdinc.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

typedef enum : u8 {
    EVENT_CPU_INSTR,
    EVENT_PIXEL,
    EVENT_DIV,
    EVENT_TIMA,
} EventKind;

struct Event {
    u64 time;
    EventKind kind;
};

static EventQueue queue_init()
{
    return (EventQueue){
        .items = nullptr,
        .len = 0,
        .capacity = 0,
    };
}

static void queue_deinit(EventQueue *queue)
{
    if (queue == nullptr)
        return;

    free(queue->items);
    *queue = queue_init();
}

[[nodiscard]] static bool queue_is_empty(const EventQueue *queue)
{
    return queue->len == 0;
}

static void queue_grow(EventQueue *queue)
{
    static size_t INIT_CAPACITY = 32;

    size_t new_capacity = 2 * queue->capacity;
    if (new_capacity < INIT_CAPACITY)
        new_capacity = INIT_CAPACITY;

    Event *new_items = realloc(queue->items, new_capacity * sizeof(*new_items));
    assert(new_items != nullptr);

    queue->items = new_items;
    queue->capacity = new_capacity;
}

static void queue_bubble_down(EventQueue *queue, size_t idx)
{
    assert(idx < queue->len);

    Event event = queue->items[idx];

    while (true) {
        size_t l = (2 * idx) + 1;
        size_t r = (2 * idx) + 2;

        if (l >= queue->len)
            break;

        size_t best = l;

        if (r < queue->len && queue->items[r].time < queue->items[l].time)
            best = r;

        if (event.time <= queue->items[best].time)
            break;

        queue->items[idx] = queue->items[best];
        idx = best;
    }

    queue->items[idx] = event;
}

static void queue_bubble_up(EventQueue *queue, size_t idx)
{
    assert(idx < queue->len);

    Event event = queue->items[idx];

    while (idx > 0) {
        size_t par = (idx - 1) / 2;

        if (queue->items[par].time <= event.time)
            break;

        queue->items[idx] = queue->items[par];
        idx = par;
    }

    queue->items[idx] = event;
}

static void queue_add(EventQueue *queue, u64 time, EventKind kind)
{

    assert(queue != nullptr);
    assert(queue->len <= queue->capacity);

    if (queue->len == queue->capacity)
        queue_grow(queue);

    assert(queue->len < queue->capacity);
    queue->items[queue->len++] = (Event){.time = time, .kind = kind};
    queue_bubble_up(queue, queue->len - 1);
}

static const Event *queue_peek(const EventQueue *queue)
{
    if (queue_is_empty(queue))
        return nullptr;

    return &queue->items[0];
}

static Event queue_remove(EventQueue *queue)
{
    assert(queue->len > 0);

    if (queue->len == 1)
        return queue->items[--queue->len];

    Event top = queue->items[0];
    queue->items[0] = queue->items[--queue->len];
    queue_bubble_down(queue, 0);
    return top;
}

[[nodiscard]] Scheduler sched_init()
{
    EventQueue queue = queue_init();
    queue_add(&queue, 0, EVENT_CPU_INSTR);
    queue_add(&queue, 0, EVENT_PIXEL);
    queue_add(&queue, 0, EVENT_DIV);
    queue_add(&queue, 0, EVENT_TIMA);

    return (Scheduler){
        .queue = queue,
    };
}

void sched_deinit(Scheduler *sched)
{
    if (sched == nullptr)
        return;

    queue_deinit(&sched->queue);
}

u64 sched_cur_time(const Scheduler *sched)
{
    const Event *next_event = queue_peek(&sched->queue);
    assert(next_event != nullptr);

    return next_event->time;
}

void sched_dispatch(Scheduler *sched, GameBoy *gb)
{
    static u64 (*const DISPATCHERS[])(GameBoy *) = {
        [EVENT_CPU_INSTR] = gb_dispatch_cpu_instr,
        [EVENT_PIXEL] = gb_dispatch_pixel,
        [EVENT_DIV] = gb_dispatch_div,
        [EVENT_TIMA] = gb_dispatch_tima,
    };

    Event event = queue_remove(&sched->queue);
    u64 elapsed_cycles = DISPATCHERS[event.kind](gb);
    queue_add(&sched->queue, event.time + elapsed_cycles, event.kind);
}

void sched_dispatch_until(Scheduler *sched, GameBoy *gb, u64 until)
{
    while (sched_cur_time(sched) < until)
        sched_dispatch(sched, gb);
}

#ifndef GEMU_SCHEDULER_H
#define GEMU_SCHEDULER_H

#include "game_boy.h"
#include <stddef.h>

typedef struct Event Event;

typedef struct {
    Event *items;
    size_t len;
    size_t capacity;
} EventQueue;

typedef struct {
    EventQueue queue;
} Scheduler;

[[nodiscard]] Scheduler sched_init();

void sched_deinit(Scheduler *sched);

[[nodiscard]] u64 sched_cur_time(const Scheduler *sched);

void sched_dispatch(Scheduler *sched, GameBoy *gb);

#endif

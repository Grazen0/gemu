#ifndef GEMU_FRONTEND_H
#define GEMU_FRONTEND_H

#include "game_boy.h"
#include <SDL3/SDL.h>

typedef struct {
    GameBoy gb;
    int window_width;
    int window_height;
    double cycle_accumulator;
    double vframe_time;
    int div_cycle_counter;
    int tima_cycle_counter;
    bool quit;
    SDL_Texture *screen_texture;
} State;

void run_until_quit(State *state, SDL_Renderer *renderer);

#endif

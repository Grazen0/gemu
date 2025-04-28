#ifndef FRONTEND_H
#define FRONTEND_H

#include "SDL3/SDL_render.h"
#include "core/game_boy.h"
typedef struct State {
    GameBoy gb;
    int window_width;
    int window_height;
    double current_time;
    double time_accumulator;
    bool quit;
    SDL_Texture* texture;
} State;

void frame(State* restrict state, SDL_Renderer* restrict renderer);

#endif

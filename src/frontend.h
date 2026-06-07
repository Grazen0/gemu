#ifndef GEMU_FRONTEND_H
#define GEMU_FRONTEND_H

#include "game_boy.h"
#include <SDL3/SDL.h>
#include <stddef.h>

typedef struct {
    GameBoy gb;
    int window_width;
    int window_height;
    bool quit;
} State;

[[nodiscard]] State state_init(const u8 *boot_rom, SDL_Window *window);

void state_deinit(State *state);

void run_until_quit(State *state, SDL_Renderer *renderer);

#endif

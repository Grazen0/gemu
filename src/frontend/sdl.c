#include "frontend/sdl.h"
#include "SDL3/SDL_timer.h"

double sdl_get_performance_time(void) {
    return (double)SDL_GetPerformanceCounter() / SDL_GetPerformanceFrequency();
}

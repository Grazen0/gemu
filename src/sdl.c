#include "sdl.h"
#include "SDL3/SDL_timer.h"

double sdl_get_performance_time(void) {
    return (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
}

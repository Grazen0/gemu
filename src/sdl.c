#include "sdl.h"
#include <SDL3/SDL.h>

double sdl_get_performance_time(void)
{
    return (double)SDL_GetPerformanceCounter() /
           (double)SDL_GetPerformanceFrequency();
}

SDL_FRect fit_rect_to_aspect_ratio(const SDL_FRect *const container,
                                   const float aspect_ratio)
{
    const double container_aspect_ratio = container->w / container->h;

    if (container_aspect_ratio > aspect_ratio) {
        // Stretched out horizontally
        const float w = container->h * aspect_ratio;
        return (SDL_FRect){
            .w = w,
            .h = container->h,
            .x = container->x + (container->w / 2.0F) - (w / 2.0F),
            .y = container->y,
        };
    }

    if (container_aspect_ratio < aspect_ratio) {
        // Stretched out vertically
        const float h = container->w / aspect_ratio;
        return (SDL_FRect){
            .w = container->w,
            .h = h,
            .x = container->x,
            .y = container->y + (container->h / 2.0F) - (h / 2.0F),
        };
    }

    // Exactly the right aspect ratio
    return *container;
}

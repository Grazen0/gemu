#ifndef GEMU_SDL_H
#define GEMU_SDL_H

#include "control.h"
#include <SDL3/SDL.h>

#define SDL_CHECKED(result, message) \
    BAIL_IF(!(result), "%s: %s", message, SDL_GetError())

[[nodiscard]] double sdl_get_performance_time(void);

[[nodiscard]] SDL_FRect fit_rect_to_aspect_ratio(const SDL_FRect *container,
                                                 float aspect_ratio);

#endif

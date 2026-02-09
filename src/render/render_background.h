#pragma once

#include <SDL3/SDL.h>
#include "utils/q_util.h"

void background_init(void);
void render_background(SDL_Renderer *renderer, f32 dt);

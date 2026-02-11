#pragma once

#include <SDL3/SDL.h>
#include "game/game.h"

void planet_textures_generate(SDL_Renderer *renderer, Game *game);
void planet_textures_destroy(Game *game);

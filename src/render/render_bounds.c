#include "render/render_bounds.h"

void render_bounds(SDL_Renderer *renderer, const Game *game) {
    const Camera *cam = &game->cam;

    f32 x0 = world_to_screen_x(cam, game->bounds_min.x);
    f32 y0 = world_to_screen_y(cam, game->bounds_max.y); // max y = top of screen (y flipped)
    f32 x1 = world_to_screen_x(cam, game->bounds_max.x);
    f32 y1 = world_to_screen_y(cam, game->bounds_min.y); // min y = bottom of screen

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 60, 60, 100);

    SDL_RenderLine(renderer, x0, y0, x1, y0); // top
    SDL_RenderLine(renderer, x1, y0, x1, y1); // right
    SDL_RenderLine(renderer, x1, y1, x0, y1); // bottom
    SDL_RenderLine(renderer, x0, y1, x0, y0); // left

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

#include "render/render_ship.h"
#include <math.h>

// Draw a filled triangle using scanline fill
static void fill_triangle(SDL_Renderer *renderer,
                           f32 x0, f32 y0,
                           f32 x1, f32 y1,
                           f32 x2, f32 y2) {
    // Sort vertices by Y
    if (y0 > y1) { f32 t; t=x0; x0=x1; x1=t; t=y0; y0=y1; y1=t; }
    if (y0 > y2) { f32 t; t=x0; x0=x2; x2=t; t=y0; y0=y2; y2=t; }
    if (y1 > y2) { f32 t; t=x1; x1=x2; x2=t; t=y1; y1=y2; y2=t; }

    if ((s32)y2 == (s32)y0) return; // degenerate

    for (s32 y = (s32)y0; y <= (s32)y2; y++) {
        f32 fy = (f32)y + 0.5f;
        f32 xa, xb;

        // Long edge: y0 -> y2
        f32 t_long = (fy - y0) / (y2 - y0);
        xa = x0 + t_long * (x2 - x0);

        // Short edge
        if (fy < y1) {
            if ((s32)y1 == (s32)y0) continue;
            f32 t_short = (fy - y0) / (y1 - y0);
            xb = x0 + t_short * (x1 - x0);
        } else {
            if ((s32)y2 == (s32)y1) continue;
            f32 t_short = (fy - y1) / (y2 - y1);
            xb = x1 + t_short * (x2 - x1);
        }

        if (xa > xb) { f32 t = xa; xa = xb; xb = t; }
        SDL_RenderLine(renderer, (s32)xa, y, (s32)xb, y);
    }
}

void render_ship(SDL_Renderer *renderer, const Game *game) {
    const Camera *cam = &game->cam;
    const Ship *ship = &game->ship;

    f32 sx = world_to_screen_x(cam, ship->pos.x);
    f32 sy = world_to_screen_y(cam, ship->pos.y);
    f32 sr = world_to_screen_r(cam, ship->radius);

    static bool logged = false;
    if (!logged) {
        SDL_Log("Ship: screen(%.1f, %.1f) r=%.1f size=%.1f", sx, sy, sr, sr * 3.0f);
        logged = true;
    }

    // Ship is a triangle pointing in the direction of ship->angle
    // Scale: ~3x the physics radius for visibility
    f32 size = sr * 3.0f;
    if (size < 8.0f) size = 8.0f;

    f32 a = ship->angle;

    // Nose (front)
    f32 nx = sx + cosf(a) * size;
    f32 ny = sy - sinf(a) * size;  // screen Y is flipped

    // Left wing
    f32 lx = sx + cosf(a + 2.4f) * size * 0.7f;
    f32 ly = sy - sinf(a + 2.4f) * size * 0.7f;

    // Right wing
    f32 rx = sx + cosf(a - 2.4f) * size * 0.7f;
    f32 ry = sy - sinf(a - 2.4f) * size * 0.7f;

    // Engine glow (small triangle behind ship)
    {
        f32 ex = sx - cosf(a) * size * 0.5f;
        f32 ey = sy + sinf(a) * size * 0.5f;

        f32 el = sx + cosf(a + 2.8f) * size * 0.3f;
        f32 ely = sy - sinf(a + 2.8f) * size * 0.3f;
        f32 er = sx + cosf(a - 2.8f) * size * 0.3f;
        f32 ery = sy - sinf(a - 2.8f) * size * 0.3f;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 160, 40, 140);
        fill_triangle(renderer, ex, ey, el, ely, er, ery);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    // Ship body
    SDL_SetRenderDrawColor(renderer, 200, 220, 255, 255);
    fill_triangle(renderer, nx, ny, lx, ly, rx, ry);

    // Ship outline
    SDL_SetRenderDrawColor(renderer, 140, 180, 255, 255);
    SDL_RenderLine(renderer, nx, ny, lx, ly);
    SDL_RenderLine(renderer, lx, ly, rx, ry);
    SDL_RenderLine(renderer, rx, ry, nx, ny);
}

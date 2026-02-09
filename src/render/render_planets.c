#include "render/render_planets.h"
#include <math.h>

// Draw a filled circle using horizontal line spans
static void fill_circle(SDL_Renderer *renderer, f32 cx, f32 cy, f32 r) {
    s32 ir = (s32)(r + 0.5f);
    for (s32 dy = -ir; dy <= ir; dy++) {
        s32 dx = (s32)(sqrtf((f32)(ir * ir - dy * dy)) + 0.5f);
        SDL_RenderLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

// Draw a circle outline (ring)
static void draw_circle(SDL_Renderer *renderer, f32 cx, f32 cy, f32 r, s32 segments) {
    f32 step = 2.0f * (f32)M_PI / segments;
    f32 px = cx + r;
    f32 py = cy;
    for (s32 i = 1; i <= segments; i++) {
        f32 a = step * i;
        f32 nx = cx + r * cosf(a);
        f32 ny = cy + r * sinf(a);
        SDL_RenderLine(renderer, px, py, nx, ny);
        px = nx;
        py = ny;
    }
}

void render_planets(SDL_Renderer *renderer, const Game *game) {
    const Camera *cam = &game->cam;
    static bool logged = false;

    for (s32 i = 0; i < game->planet_count; i++) {
        const Planet *p = &game->planets[i];
        f32 sx = world_to_screen_x(cam, p->pos.x);
        f32 sy = world_to_screen_y(cam, p->pos.y);
        f32 sr = world_to_screen_r(cam, p->radius);
        if (!logged) SDL_Log("Planet %d: screen(%.1f, %.1f) r=%.1f", i, sx, sy, sr);

        // Atmosphere glow rings (fading outward)
        for (s32 ring = 3; ring >= 1; ring--) {
            f32 ring_r = sr + ring * 4.0f;
            u8 alpha = (u8)(40 / ring);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 80, 120, 255, alpha);
            draw_circle(renderer, sx, sy, ring_r, 64);
        }

        // Planet body
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 60, 80, 140, 255);
        fill_circle(renderer, sx, sy, sr);

        // Planet surface ring
        SDL_SetRenderDrawColor(renderer, 100, 140, 200, 255);
        draw_circle(renderer, sx, sy, sr, 64);
    }

    // Goal marker — green ring
    {
        f32 gx = world_to_screen_x(cam, game->goal.pos.x);
        f32 gy = world_to_screen_y(cam, game->goal.pos.y);
        f32 gr = world_to_screen_r(cam, game->goal.radius);

        if (!logged) SDL_Log("Goal: screen(%.1f, %.1f) r=%.1f", gx, gy, gr);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 255, 100, 180);
        draw_circle(renderer, gx, gy, gr, 48);
        SDL_SetRenderDrawColor(renderer, 0, 255, 100, 60);
        draw_circle(renderer, gx, gy, gr * 0.8f, 48);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
    logged = true;
}

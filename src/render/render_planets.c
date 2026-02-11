#include "render/render_planets.h"
#include <math.h>

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

// Atmosphere glow color per planet type
static void get_atmosphere_color(PlanetType type, u8 *r, u8 *g, u8 *b) {
    switch (type) {
    case PLANET_TYPE_ROCKY:       *r = 180; *g = 160; *b = 130; break;
    case PLANET_TYPE_TERRESTRIAL: *r = 100; *g = 160; *b = 255; break;
    case PLANET_TYPE_GAS_GIANT:   *r = 220; *g = 180; *b = 100; break;
    case PLANET_TYPE_ICE:         *r = 180; *g = 210; *b = 255; break;
    case PLANET_TYPE_VOLCANIC:    *r = 255; *g = 100; *b =  40; break;
    default:                      *r =  80; *g = 120; *b = 255; break;
    }
}

void render_planets(SDL_Renderer *renderer, const Game *game) {
    const Camera *cam = &game->cam;

    for (s32 i = 0; i < game->planet_count; i++) {
        const Planet *p = &game->planets[i];
        f32 sx = world_to_screen_x(cam, p->pos.x);
        f32 sy = world_to_screen_y(cam, p->pos.y);
        f32 sr = world_to_screen_r(cam, p->radius);

        // Atmosphere glow rings (per-planet color)
        u8 ar, ag, ab;
        get_atmosphere_color(p->type, &ar, &ag, &ab);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        for (s32 ring = 3; ring >= 1; ring--) {
            f32 ring_r = sr + ring * 4.0f;
            u8 alpha = (u8)(40 / ring);
            SDL_SetRenderDrawColor(renderer, ar, ag, ab, alpha);
            draw_circle(renderer, sx, sy, ring_r, 64);
        }

        // Planet body — use generated texture if available
        if (p->texture) {
            f32 diameter = sr * 2.0f;
            SDL_FRect dst = { sx - sr, sy - sr, diameter, diameter };
            SDL_RenderTextureRotated(renderer, p->texture, NULL, &dst,
                                     (double)p->rotation_angle,
                                     NULL, SDL_FLIP_NONE);
        }
    }

    // Goal marker — green ring
    {
        f32 gx = world_to_screen_x(cam, game->goal.pos.x);
        f32 gy = world_to_screen_y(cam, game->goal.pos.y);
        f32 gr = world_to_screen_r(cam, game->goal.radius);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 255, 100, 180);
        draw_circle(renderer, gx, gy, gr, 48);
        SDL_SetRenderDrawColor(renderer, 0, 255, 100, 60);
        draw_circle(renderer, gx, gy, gr * 0.8f, 48);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

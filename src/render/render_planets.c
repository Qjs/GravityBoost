#include "render/render_planets.h"
#include <math.h>

#define RING_SEGMENTS 64
// Each segment of a ring = 2 triangles = 6 indices, 2 new verts (strip-like)
// Total: (RING_SEGMENTS + 1) * 2 verts, RING_SEGMENTS * 6 indices
#define RING_MAX_VERTS  ((RING_SEGMENTS + 1) * 2)
#define RING_MAX_INDICES (RING_SEGMENTS * 6)

// Draw a filled ring (annulus) as triangle-strip geometry
static void draw_ring(SDL_Renderer *renderer, f32 cx, f32 cy,
                       f32 inner_r, f32 outer_r, s32 segments,
                       SDL_FColor color) {
    SDL_Vertex verts[RING_MAX_VERTS];
    int indices[RING_MAX_INDICES];
    int vi = 0, ii = 0;

    f32 step = 2.0f * (f32)M_PI / segments;

    for (s32 i = 0; i <= segments; i++) {
        f32 a = step * i;
        f32 ca = cosf(a);
        f32 sa = sinf(a);

        // Outer vertex
        verts[vi].position = (SDL_FPoint){ cx + outer_r * ca, cy + outer_r * sa };
        verts[vi].color = color;
        verts[vi].tex_coord = (SDL_FPoint){ 0, 0 };
        vi++;

        // Inner vertex
        verts[vi].position = (SDL_FPoint){ cx + inner_r * ca, cy + inner_r * sa };
        verts[vi].color = color;
        verts[vi].tex_coord = (SDL_FPoint){ 0, 0 };
        vi++;

        if (i < segments) {
            int base = i * 2;
            // Triangle 1: outer[i], inner[i], outer[i+1]
            indices[ii++] = base;
            indices[ii++] = base + 1;
            indices[ii++] = base + 2;
            // Triangle 2: inner[i], inner[i+1], outer[i+1]
            indices[ii++] = base + 1;
            indices[ii++] = base + 3;
            indices[ii++] = base + 2;
        }
    }

    SDL_RenderGeometry(renderer, NULL, verts, vi, indices, ii);
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
            SDL_FColor color = {
                (f32)ar / 255.0f,
                (f32)ag / 255.0f,
                (f32)ab / 255.0f,
                (f32)alpha / 255.0f
            };
            // Ring with 1-pixel thickness to match the old line look
            draw_ring(renderer, sx, sy, ring_r - 0.5f, ring_r + 0.5f,
                      RING_SEGMENTS, color);
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

        SDL_FColor green_bright = { 0.0f, 1.0f, 100.0f / 255.0f, 180.0f / 255.0f };
        draw_ring(renderer, gx, gy, gr - 0.5f, gr + 0.5f, 48, green_bright);

        SDL_FColor green_dim = { 0.0f, 1.0f, 100.0f / 255.0f, 60.0f / 255.0f };
        f32 inner_r = gr * 0.8f;
        draw_ring(renderer, gx, gy, inner_r - 0.5f, inner_r + 0.5f, 48, green_dim);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

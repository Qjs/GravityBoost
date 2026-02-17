#include "render/render_field.h"
#include "physics/phys_gravity.h"
#include <math.h>

#define FIELD_SPACING    2.0f   // world units between sample points
#define ARROW_MIN_LEN    6.0f   // minimum arrow length in pixels
#define ARROW_MAX_LEN   28.0f   // maximum arrow length in pixels
#define ARROW_HEAD      6.0f    // arrowhead barb length in pixels
#define MAG_CLAMP        8.0f   // accel magnitude that maps to max length

// Each arrow: shaft quad (4v, 6i) + 2 barb quads (4v, 6i each) = 12 verts, 18 indices
// Budget for ~2000 arrows
#define MAX_ARROWS 2000
#define MAX_VERTS  (MAX_ARROWS * 12)
#define MAX_INDICES (MAX_ARROWS * 18)

static SDL_Vertex verts[MAX_VERTS];
static int indices[MAX_INDICES];

// Append a thin quad (line segment with thickness) to the batch
static inline void push_quad(int *vi, int *ii,
                              f32 x0, f32 y0, f32 x1, f32 y1,
                              f32 thickness, SDL_FColor color) {
    f32 dx = x1 - x0;
    f32 dy = y1 - y0;
    f32 len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;

    // Perpendicular offset
    f32 half = thickness * 0.5f;
    f32 px = -dy / len * half;
    f32 py =  dx / len * half;

    int base = *vi;

    verts[base + 0].position = (SDL_FPoint){ x0 + px, y0 + py };
    verts[base + 0].color = color;
    verts[base + 0].tex_coord = (SDL_FPoint){ 0, 0 };

    verts[base + 1].position = (SDL_FPoint){ x0 - px, y0 - py };
    verts[base + 1].color = color;
    verts[base + 1].tex_coord = (SDL_FPoint){ 0, 0 };

    verts[base + 2].position = (SDL_FPoint){ x1 + px, y1 + py };
    verts[base + 2].color = color;
    verts[base + 2].tex_coord = (SDL_FPoint){ 0, 0 };

    verts[base + 3].position = (SDL_FPoint){ x1 - px, y1 - py };
    verts[base + 3].color = color;
    verts[base + 3].tex_coord = (SDL_FPoint){ 0, 0 };

    int idx = *ii;
    indices[idx + 0] = base;
    indices[idx + 1] = base + 1;
    indices[idx + 2] = base + 2;
    indices[idx + 3] = base + 1;
    indices[idx + 4] = base + 3;
    indices[idx + 5] = base + 2;

    *vi += 4;
    *ii += 6;
}

void render_gravity_field(SDL_Renderer *renderer, const Game *game) {
    const Camera *cam = &game->cam;

    // Visible world bounds from screen corners
    f32 world_left   = screen_to_world_x(cam, 0);
    f32 world_right  = screen_to_world_x(cam, (f32)cam->screen_w);
    f32 world_top    = screen_to_world_y(cam, 0);
    f32 world_bottom = screen_to_world_y(cam, (f32)cam->screen_h);

    // Ensure min < max (screen_to_world_y flips)
    if (world_bottom > world_top) {
        f32 t = world_bottom; world_bottom = world_top; world_top = t;
    }

    // Snap to grid
    f32 x_start = floorf(world_left  / FIELD_SPACING) * FIELD_SPACING;
    f32 y_start = floorf(world_bottom / FIELD_SPACING) * FIELD_SPACING;

    int vi = 0, ii = 0;

    for (f32 wy = y_start; wy <= world_top; wy += FIELD_SPACING) {
        for (f32 wx = x_start; wx <= world_right; wx += FIELD_SPACING) {
            // Check buffer capacity (need up to 12 verts, 18 indices per arrow)
            if (vi + 12 > MAX_VERTS) goto flush;

            Vec2 pos = { wx, wy };
            Vec2 accel = gravity_accel(pos, game->planets, game->planet_count);

            f32 mag = vec2_len(accel);
            if (mag < 1e-4f) continue;

            // Normalize acceleration direction
            f32 nx = accel.x / mag;
            f32 ny = accel.y / mag;

            // Map magnitude to arrow length (pixels)
            f32 t = mag / MAG_CLAMP;
            if (t > 1.0f) t = 1.0f;
            f32 arrow_len = ARROW_MIN_LEN + t * (ARROW_MAX_LEN - ARROW_MIN_LEN);

            // Screen positions
            f32 sx = world_to_screen_x(cam, wx);
            f32 sy = world_to_screen_y(cam, wy);

            // Arrow endpoint in screen space (note: y flips)
            f32 dx =  nx * arrow_len;
            f32 dy = -ny * arrow_len;  // screen y is inverted
            f32 ex = sx + dx;
            f32 ey = sy + dy;

            // Color: lerp from dim cyan (weak) to bright yellow (strong)
            SDL_FColor color = {
                (60.0f  + t * 195.0f) / 255.0f,
                (180.0f + t * 75.0f)  / 255.0f,
                (200.0f * (1.0f - t)) / 255.0f,
                (100.0f + t * 120.0f) / 255.0f
            };

            // Shaft quad (1px thickness)
            push_quad(&vi, &ii, sx, sy, ex, ey, 1.0f, color);

            // Arrowhead barbs
            f32 screen_len = sqrtf(dx * dx + dy * dy);
            if (screen_len > 5.0f) {
                f32 ux = dx / screen_len;
                f32 uy = dy / screen_len;
                f32 ax1 = ex - ux * ARROW_HEAD + uy * ARROW_HEAD * 0.5f;
                f32 ay1 = ey - uy * ARROW_HEAD - ux * ARROW_HEAD * 0.5f;
                f32 ax2 = ex - ux * ARROW_HEAD - uy * ARROW_HEAD * 0.5f;
                f32 ay2 = ey - uy * ARROW_HEAD + ux * ARROW_HEAD * 0.5f;
                push_quad(&vi, &ii, ex, ey, ax1, ay1, 1.0f, color);
                push_quad(&vi, &ii, ex, ey, ax2, ay2, 1.0f, color);
            }
        }
    }

flush:
    if (vi > 0) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderGeometry(renderer, NULL, verts, vi, indices, ii);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

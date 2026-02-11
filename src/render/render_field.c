#include "render/render_field.h"
#include "physics/phys_gravity.h"
#include <math.h>

#define FIELD_SPACING    2.0f   // world units between sample points
#define ARROW_MIN_LEN    4.0f   // minimum arrow length in pixels
#define ARROW_MAX_LEN   30.0f   // maximum arrow length in pixels
#define ARROW_HEAD      6.0f    // arrowhead barb length in pixels
#define MAG_CLAMP       50.0f   // accel magnitude that maps to max length

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

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (f32 wy = y_start; wy <= world_top; wy += FIELD_SPACING) {
        for (f32 wx = x_start; wx <= world_right; wx += FIELD_SPACING) {
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
            u8 r = (u8)(60  + t * 195);  // 60  -> 255
            u8 g = (u8)(180 + t * 75);   // 180 -> 255
            u8 b = (u8)(200 * (1.0f - t)); // 200 -> 0
            u8 a = (u8)(60  + t * 120);  // 60  -> 180

            SDL_SetRenderDrawColor(renderer, r, g, b, a);

            // Main arrow line
            SDL_RenderLine(renderer, sx, sy, ex, ey);

            // Arrowhead barbs
            f32 screen_len = sqrtf(dx * dx + dy * dy);
            if (screen_len > 5.0f) {
                f32 ux = dx / screen_len;
                f32 uy = dy / screen_len;
                f32 ax1 = ex - ux * ARROW_HEAD + uy * ARROW_HEAD * 0.5f;
                f32 ay1 = ey - uy * ARROW_HEAD - ux * ARROW_HEAD * 0.5f;
                f32 ax2 = ex - ux * ARROW_HEAD - uy * ARROW_HEAD * 0.5f;
                f32 ay2 = ey - uy * ARROW_HEAD + ux * ARROW_HEAD * 0.5f;
                SDL_RenderLine(renderer, ex, ey, ax1, ay1);
                SDL_RenderLine(renderer, ex, ey, ax2, ay2);
            }
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

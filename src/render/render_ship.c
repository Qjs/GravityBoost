#include "render/render_ship.h"
#include <math.h>

// Draw a filled triangle using SDL_RenderGeometry (1 draw call)
static void fill_triangle(SDL_Renderer *renderer,
                           f32 x0, f32 y0,
                           f32 x1, f32 y1,
                           f32 x2, f32 y2,
                           SDL_FColor color) {
    SDL_Vertex verts[3] = {
        { .position = { x0, y0 }, .color = color, .tex_coord = { 0, 0 } },
        { .position = { x1, y1 }, .color = color, .tex_coord = { 0, 0 } },
        { .position = { x2, y2 }, .color = color, .tex_coord = { 0, 0 } },
    };
    SDL_RenderGeometry(renderer, NULL, verts, 3, NULL, 0);
}

static void draw_aim_line(SDL_Renderer *renderer, const Game *game) {
    const Camera *cam = &game->cam;
    const Ship *ship = &game->ships[0];
    const AimState *aim = &game->aim;

    f32 ship_sx = world_to_screen_x(cam, ship->pos.x);
    f32 ship_sy = world_to_screen_y(cam, ship->pos.y);
    f32 mouse_sx = world_to_screen_x(cam, aim->mouse_world.x);
    f32 mouse_sy = world_to_screen_y(cam, aim->mouse_world.y);

    // World-space distance from ship to mouse
    Vec2 dir = {
        aim->mouse_world.x - ship->pos.x,
        aim->mouse_world.y - ship->pos.y,
    };
    f32 len = vec2_len(dir);
    if (len < 0.1f) return;

    // Ratio of current drag to max velocity (for color)
    f32 ratio = len / game->vel_max;
    if (ratio > 1.0f) ratio = 1.0f;

    // Clamp the visual endpoint to vel_max distance
    f32 end_sx, end_sy;
    if (len > game->vel_max) {
        f32 nx = dir.x / len;
        f32 ny = dir.y / len;
        f32 clamp_wx = ship->pos.x + nx * game->vel_max;
        f32 clamp_wy = ship->pos.y + ny * game->vel_max;
        end_sx = world_to_screen_x(cam, clamp_wx);
        end_sy = world_to_screen_y(cam, clamp_wy);
    } else {
        end_sx = mouse_sx;
        end_sy = mouse_sy;
    }

    // Color: green at low power, yellow mid, red at max
    u8 r, g;
    if (ratio < 0.5f) {
        r = (u8)(ratio * 2.0f * 255);
        g = 255;
    } else {
        r = 255;
        g = (u8)((1.0f - ratio) * 2.0f * 255);
    }

    // Main aim line
    SDL_SetRenderDrawColor(renderer, r, g, 50, 255);
    SDL_RenderLine(renderer, ship_sx, ship_sy, end_sx, end_sy);

    // Arrowhead at the end
    f32 dx = end_sx - ship_sx;
    f32 dy = end_sy - ship_sy;
    f32 screen_len = sqrtf(dx * dx + dy * dy);
    if (screen_len > 5.0f) {
        f32 ux = dx / screen_len;
        f32 uy = dy / screen_len;
        f32 arrow = 10.0f;
        // Two barbs
        f32 ax1 = end_sx - ux * arrow + uy * arrow * 0.5f;
        f32 ay1 = end_sy - uy * arrow - ux * arrow * 0.5f;
        f32 ax2 = end_sx - ux * arrow - uy * arrow * 0.5f;
        f32 ay2 = end_sy - uy * arrow + ux * arrow * 0.5f;
        SDL_RenderLine(renderer, end_sx, end_sy, ax1, ay1);
        SDL_RenderLine(renderer, end_sx, end_sy, ax2, ay2);
    }
}

static void draw_one_ship(SDL_Renderer *renderer, const Camera *cam,
                           const Ship *ship, bool is_leader, bool is_arrived,
                           bool is_playing) {
    f32 sx = world_to_screen_x(cam, ship->pos.x);
    f32 sy = world_to_screen_y(cam, ship->pos.y);
    f32 sr = world_to_screen_r(cam, ship->radius);

    // Ship is a triangle pointing in the direction of ship->angle
    f32 size = sr * 3.0f;
    if (size < 8.0f) size = 8.0f;

    f32 a = ship->angle;

    // Nose (front)
    f32 nx = sx + cosf(a) * size;
    f32 ny = sy - sinf(a) * size;

    // Left wing
    f32 lx = sx + cosf(a + 2.4f) * size * 0.7f;
    f32 ly = sy - sinf(a + 2.4f) * size * 0.7f;

    // Right wing
    f32 rx = sx + cosf(a - 2.4f) * size * 0.7f;
    f32 ry = sy - sinf(a - 2.4f) * size * 0.7f;

    // Engine glow (only when playing and not arrived)
    if (is_playing && !is_arrived) {
        f32 ex = sx - cosf(a) * size * 0.5f;
        f32 ey = sy + sinf(a) * size * 0.5f;

        f32 el = sx + cosf(a + 2.8f) * size * 0.3f;
        f32 ely = sy - sinf(a + 2.8f) * size * 0.3f;
        f32 er = sx + cosf(a - 2.8f) * size * 0.3f;
        f32 ery = sy - sinf(a - 2.8f) * size * 0.3f;

        SDL_FColor glow_color = { 1.0f, 160.0f/255.0f, 40.0f/255.0f, 140.0f/255.0f };
        fill_triangle(renderer, ex, ey, el, ely, er, ery, glow_color);
    }

    // Ship body color
    SDL_FColor body_color;
    if (is_arrived) {
        body_color = (SDL_FColor){ 100.0f/255.0f, 1.0f, 140.0f/255.0f, 120.0f/255.0f };
    } else if (is_leader) {
        body_color = (SDL_FColor){ 200.0f/255.0f, 220.0f/255.0f, 1.0f, 1.0f };
    } else {
        body_color = (SDL_FColor){ 160.0f/255.0f, 240.0f/255.0f, 230.0f/255.0f, 1.0f };
    }
    fill_triangle(renderer, nx, ny, lx, ly, rx, ry, body_color);

    // Ship outline using SDL_RenderLines (1 call instead of 3)
    if (is_arrived) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 60, 200, 100, 120);
    } else if (is_leader) {
        SDL_SetRenderDrawColor(renderer, 140, 180, 255, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 100, 200, 200, 255);
    }
    SDL_FPoint outline[4] = {
        { nx, ny }, { lx, ly }, { rx, ry }, { nx, ny }
    };
    SDL_RenderLines(renderer, outline, 4);

    if (is_arrived) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}

void render_ship(SDL_Renderer *renderer, const Game *game) {
    const Camera *cam = &game->cam;
    bool is_playing = (game->state == GAME_STATE_PLAYING);

    // Draw tether lines from leader to alive followers
    if (game->fleet_count > 1 && game->ships[0].alive && !game->ships[0].arrived) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 80);

        f32 leader_sx = world_to_screen_x(cam, game->ships[0].pos.x);
        f32 leader_sy = world_to_screen_y(cam, game->ships[0].pos.y);

        for (s32 i = 1; i < game->fleet_count; i++) {
            if (!game->ships[i].alive || game->ships[i].arrived) continue;

            f32 fsx = world_to_screen_x(cam, game->ships[i].pos.x);
            f32 fsy = world_to_screen_y(cam, game->ships[i].pos.y);
            SDL_RenderLine(renderer, leader_sx, leader_sy, fsx, fsy);
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    // Draw each ship
    for (s32 i = 0; i < game->fleet_count; i++) {
        const Ship *ship = &game->ships[i];
        if (!ship->alive && !ship->arrived) continue;

        draw_one_ship(renderer, cam, ship, i == 0, ship->arrived, is_playing);
    }

    // Aim line (while dragging) — from leader only
    if (game->aim.aiming) {
        draw_aim_line(renderer, game);
    }
}

#pragma once
#include <stdbool.h>
#include "utils/q_util.h"

#define MAX_PLANETS 16

typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_AIM,
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,
    GAME_STATE_SUCCESS,
    GAME_STATE_FAIL,
} GameState;

typedef struct {
    f32 x, y;
} Vec2;

typedef struct {
    Vec2 pos;
    f32  radius;
    f32  mu;       // gravitational parameter
    f32  eps;      // softening parameter
} Planet;

typedef struct {
    Vec2 pos;
    Vec2 vel;
    f32  radius;
    f32  angle;    // rotation in radians
} Ship;

typedef struct {
    Vec2 pos;
    f32  radius;
} Goal;

// World-to-screen conversion
typedef struct {
    f32 ppm;       // pixels per meter
    f32 cam_x;     // camera center in world coords
    f32 cam_y;
    s32 screen_w;
    s32 screen_h;
} Camera;

typedef struct {
    GameState state;
    Camera    cam;
    Ship      ship;
    Goal      goal;
    Planet    planets[MAX_PLANETS];
    s32       planet_count;
} Game;

bool game_init(Game *game);
void game_update(Game *game, float dt);
void game_shutdown(Game *game);

// Coordinate helpers
static inline f32 world_to_screen_x(const Camera *c, f32 wx) {
    return (wx - c->cam_x) * c->ppm + c->screen_w * 0.5f;
}

static inline f32 world_to_screen_y(const Camera *c, f32 wy) {
    // Flip Y: world Y-up -> screen Y-down
    return -(wy - c->cam_y) * c->ppm + c->screen_h * 0.5f;
}

static inline f32 world_to_screen_r(const Camera *c, f32 wr) {
    return wr * c->ppm;
}

#pragma once
#include <stdbool.h>
#include <math.h>
#include <box2d/box2d.h>
#include "utils/q_util.h"

struct SDL_Texture;

#define MAX_PLANETS 16
#define MAX_FLEET   10

typedef enum {
    PLANET_TYPE_ROCKY,
    PLANET_TYPE_TERRESTRIAL,
    PLANET_TYPE_GAS_GIANT,
    PLANET_TYPE_ICE,
    PLANET_TYPE_VOLCANIC,
    PLANET_TYPE_COUNT,
} PlanetType;

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
    f32  mu;              // gravitational parameter
    f32  eps;             // softening parameter
    u32  seed;            // visual seed (derived from position if not in JSON)
    PlanetType type;      // determines color palette
    struct SDL_Texture *texture;  // generated at level load, NULL initially
    f32  rotation_speed;  // degrees/sec (derived from type)
    f32  rotation_angle;  // current angle, updated in game_update
} Planet;

typedef struct {
    Vec2 pos;
    Vec2 vel;
    f32  radius;
    f32  angle;    // rotation in radians
    bool alive;    // false = destroyed by planet collision
    bool arrived;  // true = reached goal sensor
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
    bool aiming;       // currently dragging
    Vec2 mouse_world;  // current mouse position in world coords
} AimState;

typedef struct {
    bool       active;
    b2WorldId  world;
    b2BodyId   ship_bodies[MAX_FLEET];     // one per fleet ship
    b2BodyId   goal_body;
    b2BodyId   planet_bodies[MAX_PLANETS];
} PhysState;

typedef struct {
    GameState state;
    Camera    cam;
    Ship      ships[MAX_FLEET];        // ships[0] = leader
    s32       fleet_count;             // total ships (from JSON, default 1)
    s32       required_ships;          // ships needed at goal to win (default 1)
    s32       alive_count;             // ships not destroyed
    s32       arrived_count;           // ships that reached goal
    Goal      goal;
    Planet    planets[MAX_PLANETS];
    s32       planet_count;
    f32       vel_max;
    Vec2      bounds_min;              // world-space level bounds
    Vec2      bounds_max;
    AimState  aim;
    PhysState phys;
    bool      show_field;
} Game;

bool game_init(Game *game, const char *level_path);
void game_update(Game *game, float dt);
void game_shutdown(Game *game);
void game_aim_start(Game *game, f32 screen_x, f32 screen_y);
void game_aim_move(Game *game, f32 screen_x, f32 screen_y);
void game_aim_release(Game *game, f32 screen_x, f32 screen_y);

// Coordinate helpers
static inline f32 world_to_screen_x(const Camera *c, f32 wx) {
    return (wx - c->cam_x) * c->ppm + c->screen_w * 0.5f;
}

static inline f32 world_to_screen_y(const Camera *c, f32 wy) {
    return -(wy - c->cam_y) * c->ppm + c->screen_h * 0.5f;
}

static inline f32 world_to_screen_r(const Camera *c, f32 wr) {
    return wr * c->ppm;
}

static inline f32 screen_to_world_x(const Camera *c, f32 sx) {
    return (sx - c->screen_w * 0.5f) / c->ppm + c->cam_x;
}

static inline f32 screen_to_world_y(const Camera *c, f32 sy) {
    return -(sy - c->screen_h * 0.5f) / c->ppm + c->cam_y;
}

static inline f32 vec2_len(Vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

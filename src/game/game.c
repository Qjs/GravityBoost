#include "game/game.h"
#include "physics/phys_gravity.h"

bool game_init(Game *game) {
    // Hard-coded slingshot_01 level for now
    game->state = GAME_STATE_AIM;

    game->cam = (Camera){
        .ppm      = 30.0f,
        .cam_x    = 0.0f,
        .cam_y    = 0.0f,
        .screen_w = 1280,
        .screen_h = 720,
    };

    game->ship = (Ship){
        .pos    = { -15.0f, 0.0f },
        .vel    = {   0.0f, 0.0f },
        .radius = 0.25f,
        .angle  = 0.0f,
    };

    game->goal = (Goal){
        .pos    = { 15.0f, 0.0f },
        .radius = 1.2f,
    };

    game->planet_count = 1;
    game->planets[0] = (Planet){
        .pos    = { 0.0f, 0.0f },
        .radius = 2.2f,
        .mu     = 60.0f,
        .eps    = 0.6f,
    };

    game->vel_max = 18.0f;
    game->aim = (AimState){ .aiming = false };

    return true;
}

void game_aim_start(Game *game, f32 screen_x, f32 screen_y) {
    if (game->state != GAME_STATE_AIM) return;

    game->aim.aiming = true;
    game->aim.mouse_world = (Vec2){
        screen_to_world_x(&game->cam, screen_x),
        screen_to_world_y(&game->cam, screen_y),
    };
}

void game_aim_move(Game *game, f32 screen_x, f32 screen_y) {
    if (!game->aim.aiming) return;

    game->aim.mouse_world = (Vec2){
        screen_to_world_x(&game->cam, screen_x),
        screen_to_world_y(&game->cam, screen_y),
    };
}

void game_aim_release(Game *game, f32 screen_x, f32 screen_y) {
    if (!game->aim.aiming) return;
    game->aim.aiming = false;

    f32 wx = screen_to_world_x(&game->cam, screen_x);
    f32 wy = screen_to_world_y(&game->cam, screen_y);

    // Velocity = direction from ship to release point
    Vec2 dir = {
        wx - game->ship.pos.x,
        wy - game->ship.pos.y,
    };

    f32 len = vec2_len(dir);
    if (len < 0.1f) return;  // too short, ignore

    // Clamp magnitude to vel_max
    f32 speed = len;
    if (speed > game->vel_max) speed = game->vel_max;

    game->ship.vel = (Vec2){
        dir.x / len * speed,
        dir.y / len * speed,
    };

    // Point the ship in the launch direction
    game->ship.angle = atan2f(dir.y, dir.x);

    game->state = GAME_STATE_PLAYING;
}

void game_update(Game *game, float dt) {
    if (game->state != GAME_STATE_PLAYING) return;

    // Symplectic Euler: update velocity first, then position.
    // This preserves energy much better than standard Euler for orbits.
    Vec2 accel = gravity_accel(game->ship.pos, game->planets, game->planet_count);

    // v += a * dt
    game->ship.vel.x += accel.x * dt;
    game->ship.vel.y += accel.y * dt;

    // x += v * dt  (using updated velocity)
    game->ship.pos.x += game->ship.vel.x * dt;
    game->ship.pos.y += game->ship.vel.y * dt;

    // Update ship angle to match velocity direction
    f32 speed = vec2_len(game->ship.vel);
    if (speed > 0.01f) {
        game->ship.angle = atan2f(game->ship.vel.y, game->ship.vel.x);
    }

    // Check planet collision
    for (s32 i = 0; i < game->planet_count; i++) {
        Vec2 d = {
            game->ship.pos.x - game->planets[i].pos.x,
            game->ship.pos.y - game->planets[i].pos.y,
        };
        f32 dist = vec2_len(d);
        if (dist < game->planets[i].radius + game->ship.radius) {
            game->state = GAME_STATE_FAIL;
            return;
        }
    }

    // Check goal reached
    {
        Vec2 d = {
            game->ship.pos.x - game->goal.pos.x,
            game->ship.pos.y - game->goal.pos.y,
        };
        if (vec2_len(d) < game->goal.radius) {
            game->state = GAME_STATE_SUCCESS;
            return;
        }
    }
}

void game_shutdown(Game *game) {
    (void)game;
}

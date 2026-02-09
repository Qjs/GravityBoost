#include "game/game.h"
#include "physics/physics.h"

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

    // Create Box2D world and bodies
    physics_init(game);

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

    Vec2 dir = {
        wx - game->ship.pos.x,
        wy - game->ship.pos.y,
    };

    f32 len = vec2_len(dir);
    if (len < 0.1f) return;

    f32 speed = len;
    if (speed > game->vel_max) speed = game->vel_max;

    Vec2 vel = {
        dir.x / len * speed,
        dir.y / len * speed,
    };

    // Set velocity on the Box2D body
    physics_launch(game, vel);

    game->ship.vel = vel;
    game->ship.angle = atan2f(dir.y, dir.x);
    game->state = GAME_STATE_PLAYING;
}

void game_update(Game *game, float dt) {
    // Box2D handles integration, collisions, and goal detection
    physics_step(game, dt);
}

void game_shutdown(Game *game) {
    physics_shutdown(game);
}

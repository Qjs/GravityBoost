#include "game/game.h"
#include "data/json.h"
#include "physics/physics.h"
#include <math.h>

bool game_init(Game *game, const char *level_path) {
    game->state = GAME_STATE_AIM;

    // Screen defaults
    game->cam.cam_x    = 0.0f;
    game->cam.cam_y    = 0.0f;
    game->cam.screen_w = 1280;
    game->cam.screen_h = 720;

    // Fleet defaults (before JSON overrides)
    game->fleet_count    = 1;
    game->required_ships = 1;

    // Load level data from JSON
    if (!json_load(level_path, game))
        return false;

    // Initialize fleet ships in circular formation around leader
    Vec2 start_pos = game->ships[0].pos;
    f32  ship_radius = game->ships[0].radius;

    for (s32 i = 0; i < game->fleet_count; i++) {
        game->ships[i].vel     = (Vec2){ 0.0f, 0.0f };
        game->ships[i].angle   = 0.0f;
        game->ships[i].radius  = ship_radius;
        game->ships[i].alive   = true;
        game->ships[i].arrived = false;

        if (i > 0) {
            // Place followers in a semicircle behind the leader
            s32 n = game->fleet_count - 1;
            f32 spread = (f32)M_PI;  // 180 degree arc behind leader
            f32 angle = (f32)M_PI - spread * 0.5f + spread * (f32)(i - 1) / (f32)(n > 1 ? n - 1 : 1);
            game->ships[i].pos.x = start_pos.x + cosf(angle) * 1.0f;
            game->ships[i].pos.y = start_pos.y + sinf(angle) * 1.0f;
        }
    }

    game->alive_count   = game->fleet_count;
    game->arrived_count = 0;
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
        wx - game->ships[0].pos.x,
        wy - game->ships[0].pos.y,
    };

    f32 len = vec2_len(dir);
    if (len < 0.1f) return;

    f32 speed = len;
    if (speed > game->vel_max) speed = game->vel_max;

    Vec2 vel = {
        dir.x / len * speed,
        dir.y / len * speed,
    };

    // Set velocity on the leader Box2D body (followers follow via springs)
    physics_launch(game, vel);

    game->ships[0].vel = vel;
    game->ships[0].angle = atan2f(dir.y, dir.x);
    game->state = GAME_STATE_PLAYING;
}

void game_update(Game *game, float dt) {
    // Box2D handles integration, collisions, and goal detection
    physics_step(game, dt);

    // Rotate planet textures
    for (s32 i = 0; i < game->planet_count; i++) {
        game->planets[i].rotation_angle += game->planets[i].rotation_speed * dt;
        if (game->planets[i].rotation_angle >= 360.0f)
            game->planets[i].rotation_angle -= 360.0f;
    }
}

void game_shutdown(Game *game) {
    physics_shutdown(game);
}

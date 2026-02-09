#include "game/game.h"

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

    return true;
}

void game_update(Game *game, float dt) {
    (void)game;
    (void)dt;
}

void game_shutdown(Game *game) {
    (void)game;
}

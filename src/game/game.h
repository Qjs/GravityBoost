#pragma once
#include <stdbool.h>

typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,
} GameState;

typedef struct {
    GameState state;
} Game;

bool game_init(Game *game);
void game_update(Game *game, float dt);
void game_shutdown(Game *game);

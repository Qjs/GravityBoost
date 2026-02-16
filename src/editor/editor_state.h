#pragma once

#include "game/game.h"

#define SEL_NONE  -1
#define SEL_START 100
#define SEL_GOAL  101

typedef struct {
    // The game struct used for rendering — contains planets, goal, bounds, cam
    Game game;

    // Level metadata not stored in Game struct
    char name[64];
    f32  ship_density;
    f32  ship_restitution;
    bool show_field_default;
    bool allow_sink, allow_repel;
    s32  allow_max;

    // UI state
    s32  selected;        // SEL_NONE, planet index, SEL_START, or SEL_GOAL
    s32  hovered;
    bool dragging;
    Vec2 drag_offset;     // world-space offset from cursor to object center
    bool adding_planet;   // true = next click places a new planet
    char file_path[256];  // current file path for save/load
    bool textures_dirty;  // true = need to regenerate planet textures
} EditorState;

void editor_state_defaults(EditorState *es);

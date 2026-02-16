#include "editor/editor_state.h"
#include <string.h>
#include <stdio.h>

void editor_state_defaults(EditorState *es) {
    memset(es, 0, sizeof(*es));

    snprintf(es->name, sizeof(es->name), "New Level");

    // Camera / view
    es->game.cam.ppm      = 30.0f;
    es->game.cam.screen_w = 1280;
    es->game.cam.screen_h = 720;

    // Bounds
    es->game.bounds_min = (Vec2){ -20.0f, -12.0f };
    es->game.bounds_max = (Vec2){  20.0f,  12.0f };

    // Start
    es->game.ships[0].pos    = (Vec2){ -15.0f, 0.0f };
    es->game.ships[0].radius = 0.25f;
    es->game.vel_max          = 18.0f;

    // Goal
    es->game.goal.pos    = (Vec2){ 15.0f, 0.0f };
    es->game.goal.radius = 1.2f;

    // Ship physics
    es->ship_density     = 1.0f;
    es->ship_restitution = 0.1f;

    // Fleet
    es->game.fleet_count    = 1;
    es->game.required_ships = 1;

    // Allow place
    es->allow_sink  = true;
    es->allow_repel = true;
    es->allow_max   = 3;

    // UI
    es->selected = SEL_NONE;
    es->hovered  = SEL_NONE;
}

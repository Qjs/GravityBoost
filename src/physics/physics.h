#pragma once

#include "game/game.h"
#include <stdint.h>

// Shape user data tags for identifying collisions
// Ships use (void*)(uintptr_t)(100 + ship_index)
#define PHYS_TAG_SHIP_BASE 100
#define PHYS_TAG_PLANET ((void*)(uintptr_t)2)
#define PHYS_TAG_GOAL   ((void*)(uintptr_t)3)

static inline bool phys_tag_is_ship(void *tag) {
    uintptr_t t = (uintptr_t)tag;
    return t >= PHYS_TAG_SHIP_BASE && t < PHYS_TAG_SHIP_BASE + MAX_FLEET;
}

static inline int phys_tag_ship_index(void *tag) {
    return (int)((uintptr_t)tag - PHYS_TAG_SHIP_BASE);
}

// Create Box2D world and bodies for all game objects
void physics_init(Game *game);

// Apply gravity forces, step world, sync state back, check collisions
void physics_step(Game *game, f32 dt);

// Set leader ship velocity (called on launch)
void physics_launch(Game *game, Vec2 vel);

// Destroy Box2D world and all bodies
void physics_shutdown(Game *game);

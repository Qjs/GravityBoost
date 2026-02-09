#pragma once

#include "game/game.h"

// Shape user data tags for identifying collisions
#define PHYS_TAG_SHIP   ((void*)(uintptr_t)1)
#define PHYS_TAG_PLANET ((void*)(uintptr_t)2)
#define PHYS_TAG_GOAL   ((void*)(uintptr_t)3)

// Create Box2D world and bodies for all game objects
void physics_init(Game *game);

// Apply gravity forces, step world, sync state back, check collisions
void physics_step(Game *game, f32 dt);

// Set ship velocity (called on launch)
void physics_launch(Game *game, Vec2 vel);

// Destroy Box2D world and all bodies
void physics_shutdown(Game *game);

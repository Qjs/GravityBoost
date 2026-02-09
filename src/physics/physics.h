#pragma once
#include <stdbool.h>
#include <box2d/box2d.h>

typedef struct {
    b2WorldId world_id;
} PhysicsWorld;

bool physics_create(PhysicsWorld *pw);
void physics_destroy(PhysicsWorld *pw);
void physics_step(PhysicsWorld *pw, float dt);

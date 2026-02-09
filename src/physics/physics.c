#include "physics/physics.h"
#include "physics/phys_gravity.h"
#include <stdint.h>

void physics_init(Game *game) {
    PhysState *ps = &game->phys;

    // Create world with zero gravity (we apply our own softened model)
    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity = (b2Vec2){ 0.0f, 0.0f };
    ps->world = b2CreateWorld(&world_def);

    // --- Ship (dynamic, bullet for CCD) ---
    {
        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type = b2_dynamicBody;
        body_def.position = (b2Vec2){ game->ship.pos.x, game->ship.pos.y };
        body_def.isBullet = true;
        ps->ship_body = b2CreateBody(ps->world, &body_def);

        b2ShapeDef shape_def = b2DefaultShapeDef();
        shape_def.density = 1.0f;
        shape_def.userData = PHYS_TAG_SHIP;
        shape_def.enableContactEvents = true;
        shape_def.enableSensorEvents = true;

        b2Circle circle = { .center = { 0, 0 }, .radius = game->ship.radius };
        b2CreateCircleShape(ps->ship_body, &shape_def, &circle);
    }

    // --- Planets (static, solid colliders) ---
    for (s32 i = 0; i < game->planet_count; i++) {
        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type = b2_staticBody;
        body_def.position = (b2Vec2){ game->planets[i].pos.x, game->planets[i].pos.y };
        ps->planet_bodies[i] = b2CreateBody(ps->world, &body_def);

        b2ShapeDef shape_def = b2DefaultShapeDef();
        shape_def.userData = PHYS_TAG_PLANET;
        shape_def.enableContactEvents = true;

        b2Circle circle = { .center = { 0, 0 }, .radius = game->planets[i].radius };
        b2CreateCircleShape(ps->planet_bodies[i], &shape_def, &circle);
    }

    // --- Goal (static, sensor) ---
    {
        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type = b2_staticBody;
        body_def.position = (b2Vec2){ game->goal.pos.x, game->goal.pos.y };
        ps->goal_body = b2CreateBody(ps->world, &body_def);

        b2ShapeDef shape_def = b2DefaultShapeDef();
        shape_def.userData = PHYS_TAG_GOAL;
        shape_def.isSensor = true;
        shape_def.enableSensorEvents = true;

        b2Circle circle = { .center = { 0, 0 }, .radius = game->goal.radius };
        b2CreateCircleShape(ps->goal_body, &shape_def, &circle);
    }

    ps->active = true;
}

void physics_launch(Game *game, Vec2 vel) {
    b2Body_SetLinearVelocity(game->phys.ship_body, (b2Vec2){ vel.x, vel.y });
}

void physics_step(Game *game, f32 dt) {
    PhysState *ps = &game->phys;
    if (!ps->active) return;
    if (game->state != GAME_STATE_PLAYING) return;

    // Apply our softened gravity as a force: F = m * a
    b2Vec2 b2pos = b2Body_GetPosition(ps->ship_body);
    Vec2 pos = { b2pos.x, b2pos.y };

    Vec2 accel = gravity_accel(pos, game->planets, game->planet_count);
    f32 mass = b2Body_GetMass(ps->ship_body);
    b2Vec2 force = { mass * accel.x, mass * accel.y };
    b2Body_ApplyForceToCenter(ps->ship_body, force, true);

    // Step the Box2D world
    b2World_Step(ps->world, dt, 4);

    // Sync position and velocity back to game state
    b2Vec2 new_pos = b2Body_GetPosition(ps->ship_body);
    b2Vec2 new_vel = b2Body_GetLinearVelocity(ps->ship_body);
    game->ship.pos = (Vec2){ new_pos.x, new_pos.y };
    game->ship.vel = (Vec2){ new_vel.x, new_vel.y };

    // Update ship angle from velocity
    f32 speed = vec2_len(game->ship.vel);
    if (speed > 0.01f) {
        game->ship.angle = atan2f(game->ship.vel.y, game->ship.vel.x);
    }

    // Check contact events (ship hitting a planet = FAIL)
    b2ContactEvents contacts = b2World_GetContactEvents(ps->world);
    for (int i = 0; i < contacts.beginCount; i++) {
        void *tagA = b2Shape_GetUserData(contacts.beginEvents[i].shapeIdA);
        void *tagB = b2Shape_GetUserData(contacts.beginEvents[i].shapeIdB);

        bool ship_involved   = (tagA == PHYS_TAG_SHIP)   || (tagB == PHYS_TAG_SHIP);
        bool planet_involved = (tagA == PHYS_TAG_PLANET)  || (tagB == PHYS_TAG_PLANET);

        if (ship_involved && planet_involved) {
            game->state = GAME_STATE_FAIL;
            return;
        }
    }

    // Check sensor events (ship entering goal = SUCCESS)
    b2SensorEvents sensors = b2World_GetSensorEvents(ps->world);
    for (int i = 0; i < sensors.beginCount; i++) {
        void *sensor_tag  = b2Shape_GetUserData(sensors.beginEvents[i].sensorShapeId);
        void *visitor_tag = b2Shape_GetUserData(sensors.beginEvents[i].visitorShapeId);

        if (sensor_tag == PHYS_TAG_GOAL && visitor_tag == PHYS_TAG_SHIP) {
            game->state = GAME_STATE_SUCCESS;
            return;
        }
    }
}

void physics_shutdown(Game *game) {
    PhysState *ps = &game->phys;
    if (!ps->active) return;

    b2DestroyWorld(ps->world);
    ps->active = false;
}

#include "physics/physics.h"
#include "physics/phys_gravity.h"
#include <stdint.h>
#include <math.h>

// --- Separation force helpers (from scratch_ideas.c) ---

static inline float clampf(float x, float a, float b) {
    return x < a ? a : (x > b ? b : x);
}

static void fleet_apply_separation(
    b2BodyId *bodies, const bool *alive, int count,
    float r_sep, float strength, float max_accel)
{
    float r2 = r_sep * r_sep;

    for (int i = 0; i < count; i++) {
        if (!alive[i]) continue;

        b2Vec2 pi = b2Body_GetPosition(bodies[i]);
        b2Vec2 accel = {0, 0};

        for (int j = 0; j < count; j++) {
            if (j == i || !alive[j]) continue;

            b2Vec2 pj = b2Body_GetPosition(bodies[j]);
            b2Vec2 d = { pi.x - pj.x, pi.y - pj.y };
            float d2 = d.x * d.x + d.y * d.y;

            if (d2 >= r2) continue;

            float dist = sqrtf(d2 + 1e-6f);
            float t = 1.0f - (dist / r_sep);
            float w = t * t;

            float inv = 1.0f / (dist + 1e-6f);
            accel.x += d.x * inv * strength * w;
            accel.y += d.y * inv * strength * w;
        }

        float a_len = sqrtf(accel.x * accel.x + accel.y * accel.y);
        if (a_len > max_accel) {
            float s = max_accel / (a_len + 1e-6f);
            accel.x *= s;
            accel.y *= s;
        }

        float m = b2Body_GetMass(bodies[i]);
        b2Vec2 F = { accel.x * m, accel.y * m };
        b2Body_ApplyForceToCenter(bodies[i], F, true);
    }
}

// Apply spring force ONLY to followers, pulling them toward the leader.
// Leader feels no reaction force.
static void fleet_apply_tether(
    b2BodyId *bodies, const bool *active, int count,
    float rest_length, float hertz, float damping_ratio)
{
    if (count < 2 || !active[0]) return;

    b2Vec2 leader_pos = b2Body_GetPosition(bodies[0]);
    b2Vec2 leader_vel = b2Body_GetLinearVelocity(bodies[0]);

    // Convert hertz + damping ratio to spring constant and damping coefficient
    // Using: k = (2*pi*hertz)^2 * mass,  c = 2*damping_ratio*(2*pi*hertz)*mass
    for (int i = 1; i < count; i++) {
        if (!active[i]) continue;

        b2Vec2 fpos = b2Body_GetPosition(bodies[i]);
        b2Vec2 fvel = b2Body_GetLinearVelocity(bodies[i]);
        float mass = b2Body_GetMass(bodies[i]);

        float dx = leader_pos.x - fpos.x;
        float dy = leader_pos.y - fpos.y;
        float dist = sqrtf(dx * dx + dy * dy + 1e-6f);
        float stretch = dist - rest_length;

        // Only pull when beyond rest length (no push when close)
        if (stretch <= 0.0f) continue;

        float nx = dx / dist;
        float ny = dy / dist;

        float omega = 2.0f * (float)M_PI * hertz;
        float k = omega * omega * mass;
        float c = 2.0f * damping_ratio * omega * mass;

        // Relative velocity along tether axis
        float rel_vx = fvel.x - leader_vel.x;
        float rel_vy = fvel.y - leader_vel.y;
        float rel_v_along = rel_vx * nx + rel_vy * ny;

        // Spring + damping force (applied to follower only)
        float f_mag = k * stretch - c * rel_v_along;
        b2Vec2 F = { f_mag * nx, f_mag * ny };
        b2Body_ApplyForceToCenter(bodies[i], F, true);
    }
}

// --- Physics init ---

void physics_init(Game *game) {
    PhysState *ps = &game->phys;

    // Create world with zero gravity (we apply our own softened model)
    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity = (b2Vec2){ 0.0f, 0.0f };
    ps->world = b2CreateWorld(&world_def);

    // --- Fleet ships (dynamic, bullet for CCD) ---
    for (s32 i = 0; i < game->fleet_count; i++) {
        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type = b2_dynamicBody;
        body_def.position = (b2Vec2){ game->ships[i].pos.x, game->ships[i].pos.y };
        body_def.isBullet = true;
        ps->ship_bodies[i] = b2CreateBody(ps->world, &body_def);

        b2ShapeDef shape_def = b2DefaultShapeDef();
        shape_def.density = 1.0f;
        shape_def.userData = (void*)(uintptr_t)(PHYS_TAG_SHIP_BASE + i);
        shape_def.enableContactEvents = true;
        shape_def.enableSensorEvents = true;

        b2Circle circle = { .center = { 0, 0 }, .radius = game->ships[i].radius };
        b2CreateCircleShape(ps->ship_bodies[i], &shape_def, &circle);
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
    // Only set velocity on the leader — followers follow via springs
    b2Body_SetLinearVelocity(game->phys.ship_bodies[0], (b2Vec2){ vel.x, vel.y });
}

void physics_step(Game *game, f32 dt) {
    PhysState *ps = &game->phys;
    if (!ps->active) return;
    if (game->state != GAME_STATE_PLAYING) return;

    // Collect alive flags for separation
    bool alive_flags[MAX_FLEET];
    for (s32 i = 0; i < game->fleet_count; i++)
        alive_flags[i] = game->ships[i].alive && !game->ships[i].arrived;

    // Apply gravity to all alive ships
    for (s32 i = 0; i < game->fleet_count; i++) {
        if (!alive_flags[i]) continue;

        b2Vec2 b2pos = b2Body_GetPosition(ps->ship_bodies[i]);
        Vec2 pos = { b2pos.x, b2pos.y };

        Vec2 accel = gravity_accel(pos, game->planets, game->planet_count);
        f32 mass = b2Body_GetMass(ps->ship_bodies[i]);
        b2Vec2 force = { mass * accel.x, mass * accel.y };
        b2Body_ApplyForceToCenter(ps->ship_bodies[i], force, true);
    }

    // Apply one-way spring tether (followers pulled toward leader, leader unaffected)
    fleet_apply_tether(ps->ship_bodies, alive_flags, game->fleet_count,
                       1.5f, 1.0f, 0.5f);

    // Apply separation force between alive fleet ships
    fleet_apply_separation(ps->ship_bodies, alive_flags, game->fleet_count,
                           2.0f, 30.0f, 15.0f);

    // Step the Box2D world
    b2World_Step(ps->world, dt, 4);

    // Sync position and velocity back to game state for all alive ships
    for (s32 i = 0; i < game->fleet_count; i++) {
        if (!game->ships[i].alive) continue;
        if (game->ships[i].arrived) continue;

        b2Vec2 new_pos = b2Body_GetPosition(ps->ship_bodies[i]);
        b2Vec2 new_vel = b2Body_GetLinearVelocity(ps->ship_bodies[i]);
        game->ships[i].pos = (Vec2){ new_pos.x, new_pos.y };
        game->ships[i].vel = (Vec2){ new_vel.x, new_vel.y };

        f32 speed = vec2_len(game->ships[i].vel);
        if (speed > 0.01f) {
            game->ships[i].angle = atan2f(game->ships[i].vel.y, game->ships[i].vel.x);
        }
    }

    // Check contact events (ship hitting a planet = destroy that ship)
    b2ContactEvents contacts = b2World_GetContactEvents(ps->world);
    for (int i = 0; i < contacts.beginCount; i++) {
        void *tagA = b2Shape_GetUserData(contacts.beginEvents[i].shapeIdA);
        void *tagB = b2Shape_GetUserData(contacts.beginEvents[i].shapeIdB);

        void *ship_tag = NULL;
        bool planet_hit = false;

        if (phys_tag_is_ship(tagA) && tagB == PHYS_TAG_PLANET) {
            ship_tag = tagA;
            planet_hit = true;
        } else if (phys_tag_is_ship(tagB) && tagA == PHYS_TAG_PLANET) {
            ship_tag = tagB;
            planet_hit = true;
        }

        if (planet_hit) {
            int idx = phys_tag_ship_index(ship_tag);
            if (idx >= 0 && idx < game->fleet_count && game->ships[idx].alive) {
                game->ships[idx].alive = false;
                game->alive_count--;

                // Disable the body
                b2Body_Disable(ps->ship_bodies[idx]);

                // Check if impossible to win
                if (game->alive_count < game->required_ships) {
                    game->state = GAME_STATE_FAIL;
                    return;
                }
            }
        }
    }

    // Check sensor events (ship entering goal)
    b2SensorEvents sensors = b2World_GetSensorEvents(ps->world);
    for (int i = 0; i < sensors.beginCount; i++) {
        void *sensor_tag  = b2Shape_GetUserData(sensors.beginEvents[i].sensorShapeId);
        void *visitor_tag = b2Shape_GetUserData(sensors.beginEvents[i].visitorShapeId);

        if (sensor_tag == PHYS_TAG_GOAL && phys_tag_is_ship(visitor_tag)) {
            int idx = phys_tag_ship_index(visitor_tag);
            if (idx >= 0 && idx < game->fleet_count &&
                game->ships[idx].alive && !game->ships[idx].arrived) {

                game->ships[idx].arrived = true;
                game->arrived_count++;

                // Disable body (docked at goal)
                b2Body_Disable(ps->ship_bodies[idx]);

                if (game->arrived_count >= game->required_ships) {
                    game->state = GAME_STATE_SUCCESS;
                    return;
                }
            }
        }
    }
}

void physics_shutdown(Game *game) {
    PhysState *ps = &game->phys;
    if (!ps->active) return;

    b2DestroyWorld(ps->world);
    ps->active = false;
}

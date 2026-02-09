#include "physics/phys_gravity.h"
#include <math.h>

Vec2 gravity_accel(Vec2 pos, const Planet *planets, s32 planet_count) {
    Vec2 accel = { 0.0f, 0.0f };

    for (s32 i = 0; i < planet_count; i++) {
        const Planet *p = &planets[i];

        // Vector from ship to planet: (p_k - x)
        f32 dx = p->pos.x - pos.x;
        f32 dy = p->pos.y - pos.y;

        // ||p_k - x||² + ε_k²
        f32 dist_sq = dx * dx + dy * dy;
        f32 soft_sq = dist_sq + p->eps * p->eps;

        // (||p_k - x||² + ε_k²)^(3/2)
        f32 denom = soft_sq * sqrtf(soft_sq);

        // μ_k * (p_k - x) / denom
        f32 scale = p->mu / denom;
        accel.x += scale * dx;
        accel.y += scale * dy;
    }

    return accel;
}

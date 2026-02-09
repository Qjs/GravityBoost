#pragma once

#include "game/game.h"

// Compute net gravitational acceleration on a point at `pos`
// from all planets in the game.
//
// Softened gravity model:
//   a(x) = Σ_k  μ_k * (p_k - x) / (||p_k - x||² + ε_k²)^(3/2)
//
Vec2 gravity_accel(Vec2 pos, const Planet *planets, s32 planet_count);

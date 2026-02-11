#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"
#include "render/planet_gen.h"
#include <math.h>

#define TEX_SIZE 256

// RGBA color
typedef struct { u8 r, g, b, a; } Color;

// Each palette has 4 color stops
typedef struct {
    Color stops[4];
    Color atmosphere;
} PlanetPalette;

static const PlanetPalette palettes[PLANET_TYPE_COUNT] = {
    // ROCKY: browns and grays
    [PLANET_TYPE_ROCKY] = {
        .stops = { {80,70,60,255}, {140,120,100,255}, {100,90,80,255}, {170,160,140,255} },
        .atmosphere = {180,160,130,160},
    },
    // TERRESTRIAL: blues and greens
    [PLANET_TYPE_TERRESTRIAL] = {
        .stops = { {30,60,140,255}, {40,120,80,255}, {60,140,60,255}, {50,90,160,255} },
        .atmosphere = {100,160,255,160},
    },
    // GAS GIANT: oranges and tans with bands
    [PLANET_TYPE_GAS_GIANT] = {
        .stops = { {180,130,60,255}, {200,160,90,255}, {160,110,50,255}, {220,180,120,255} },
        .atmosphere = {220,180,100,140},
    },
    // ICE: pale blues and whites
    [PLANET_TYPE_ICE] = {
        .stops = { {180,200,230,255}, {140,170,210,255}, {200,220,240,255}, {160,190,220,255} },
        .atmosphere = {180,210,255,160},
    },
    // VOLCANIC: blacks and oranges
    [PLANET_TYPE_VOLCANIC] = {
        .stops = { {40,20,20,255}, {180,60,20,255}, {60,30,10,255}, {220,100,30,255} },
        .atmosphere = {255,100,40,160},
    },
};

static Color color_lerp(Color a, Color b, f32 t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return (Color){
        (u8)(a.r + (b.r - a.r) * t),
        (u8)(a.g + (b.g - a.g) * t),
        (u8)(a.b + (b.b - a.b) * t),
        (u8)(a.a + (b.a - a.a) * t),
    };
}

// Map a noise value [0,1] to a 4-stop gradient
static Color palette_sample(const PlanetPalette *pal, f32 t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    f32 scaled = t * 3.0f; // 4 stops = 3 segments
    int seg = (int)scaled;
    if (seg > 2) seg = 2;
    f32 frac = scaled - (f32)seg;
    return color_lerp(pal->stops[seg], pal->stops[seg + 1], frac);
}

static void generate_planet_texture(SDL_Renderer *renderer, Planet *planet) {
    SDL_Texture *tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                         SDL_TEXTUREACCESS_STREAMING,
                                         TEX_SIZE, TEX_SIZE);
    if (!tex) {
        SDL_Log("planet_gen: failed to create texture: %s", SDL_GetError());
        return;
    }

    void *pixels;
    int pitch;
    if (!SDL_LockTexture(tex, NULL, &pixels, &pitch)) {
        SDL_Log("planet_gen: failed to lock texture: %s", SDL_GetError());
        SDL_DestroyTexture(tex);
        return;
    }

    const PlanetPalette *pal = &palettes[planet->type];
    f32 seed_offset = (f32)(planet->seed % 1000);
    f32 half = TEX_SIZE * 0.5f;

    for (int y = 0; y < TEX_SIZE; y++) {
        u8 *row = (u8 *)pixels + y * pitch;
        for (int x = 0; x < TEX_SIZE; x++) {
            // Normalized coords [-1, 1]
            f32 nx = (x + 0.5f - half) / half;
            f32 ny = (y + 0.5f - half) / half;
            f32 dist2 = nx * nx + ny * ny;

            if (dist2 > 1.0f) {
                // Outside circle — transparent
                row[x * 4 + 0] = 0;
                row[x * 4 + 1] = 0;
                row[x * 4 + 2] = 0;
                row[x * 4 + 3] = 0;
                continue;
            }

            f32 dist = sqrtf(dist2);

            // Spherical projection: map 2D to 3D sphere surface
            f32 nz = sqrtf(1.0f - dist2);
            f32 sx = nx + seed_offset;
            f32 sy = ny + seed_offset;
            f32 sz = nz + seed_offset;

            // Base terrain noise (fBm, 6 octaves)
            f32 noise_val = stb_perlin_fbm_noise3(
                sx * 2.0f, sy * 2.0f, sz * 2.0f,
                2.0f, 0.5f, 6);
            // Normalize from ~[-1,1] to [0,1]
            noise_val = noise_val * 0.5f + 0.5f;

            // Gas giant banding
            if (planet->type == PLANET_TYPE_GAS_GIANT) {
                f32 band = sinf(ny * 12.0f + stb_perlin_noise3(
                    sx * 3.0f, sy * 3.0f, sz * 3.0f, 0, 0, 0) * 2.0f);
                noise_val = noise_val * 0.4f + (band * 0.5f + 0.5f) * 0.6f;
            }

            // Surface detail (turbulence at higher frequency)
            f32 detail = stb_perlin_turbulence_noise3(
                sx * 5.0f, sy * 5.0f, sz * 5.0f,
                2.0f, 0.5f, 3);
            noise_val += detail * 0.15f;
            if (noise_val < 0.0f) noise_val = 0.0f;
            if (noise_val > 1.0f) noise_val = 1.0f;

            // Map to palette
            Color col = palette_sample(pal, noise_val);

            // Directional lighting (top-left light source)
            f32 light_x = -0.5f, light_y = -0.5f, light_z = 0.707f;
            f32 dot = nx * light_x + ny * light_y + nz * light_z;
            if (dot < 0.0f) dot = 0.0f;
            f32 lighting = 0.3f + 0.7f * dot; // ambient 0.3 + diffuse 0.7

            // Limb darkening
            f32 limb = 1.0f - dist * dist * dist;
            f32 brightness = lighting * limb;

            col.r = (u8)(col.r * brightness);
            col.g = (u8)(col.g * brightness);
            col.b = (u8)(col.b * brightness);

            // Atmospheric rim glow
            if (dist > 0.85f) {
                f32 rim_t = (dist - 0.85f) / 0.15f; // 0..1
                rim_t = rim_t * rim_t; // ease in
                col = color_lerp(col, pal->atmosphere, rim_t * 0.7f);
                // Ensure fully opaque
                col.a = 255;
            }

            row[x * 4 + 0] = col.r;
            row[x * 4 + 1] = col.g;
            row[x * 4 + 2] = col.b;
            row[x * 4 + 3] = col.a;
        }
    }

    SDL_UnlockTexture(tex);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);

    planet->texture = tex;
}

void planet_textures_generate(SDL_Renderer *renderer, Game *game) {
    for (s32 i = 0; i < game->planet_count; i++) {
        generate_planet_texture(renderer, &game->planets[i]);
        SDL_Log("planet_gen: planet %d seed=%u type=%d", i,
                game->planets[i].seed, game->planets[i].type);
    }
}

void planet_textures_destroy(Game *game) {
    for (s32 i = 0; i < game->planet_count; i++) {
        if (game->planets[i].texture) {
            SDL_DestroyTexture(game->planets[i].texture);
            game->planets[i].texture = NULL;
        }
    }
}

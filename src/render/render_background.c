#include "render/render_background.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_W 1280
#define SCREEN_H 720

#define NUM_LAYERS 3
#define STARS_PER_LAYER 60

typedef struct {
    SDL_Texture *texture;
    f32  speed;      // pixels per second (vertical scroll)
} StarLayer;

static StarLayer layers[NUM_LAYERS];
static bool initialized = false;
static u64 start_counter = 0;  // absolute time origin for scroll

static f32 randf(void) {
    return (f32)rand() / (f32)RAND_MAX;
}

// Stamp a star (filled square) into an RGBA pixel buffer
static void stamp_star(u32 *pixels, int pitch_pixels,
                        int cx, int cy, int size,
                        u8 r, u8 g, u8 b) {
    int half = size / 2;
    for (int dy = -half; dy < size - half; dy++) {
        int py = cy + dy;
        if (py < 0 || py >= SCREEN_H) continue;
        for (int dx = -half; dx < size - half; dx++) {
            int px = cx + dx;
            if (px < 0 || px >= SCREEN_W) continue;
            // RGBA8888: R in low byte
            pixels[py * pitch_pixels + px] = (u32)r | ((u32)g << 8) | ((u32)b << 16) | (0xFFu << 24);
        }
    }
}

void background_init(SDL_Renderer *renderer) {
    const f32 speeds[] = { 15.0f, 35.0f, 70.0f };
    const int sizes[]  = {     1,     2,     3 };
    const u8  brights[] = {   80,   140,   220 };

    for (int l = 0; l < NUM_LAYERS; l++) {
        layers[l].speed  = speeds[l];

        // Compute layer color (same tint formula as before)
        u8 bright = brights[l];
        u8 cr = (u8)(bright * (0.7f + 0.3f * ((f32)l / (NUM_LAYERS - 1))));
        u8 cg = (u8)(bright * (0.8f + 0.2f * ((f32)l / (NUM_LAYERS - 1))));
        u8 cb = bright;

        // Create streaming texture with alpha
        layers[l].texture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING,
            SCREEN_W, SCREEN_H);
        SDL_SetTextureBlendMode(layers[l].texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(layers[l].texture, SDL_SCALEMODE_NEAREST);

        // Lock, clear to transparent, stamp stars, unlock
        void *raw_pixels;
        int pitch;
        SDL_LockTexture(layers[l].texture, NULL, &raw_pixels, &pitch);

        int pitch_pixels = pitch / 4;
        u32 *pixels = (u32 *)raw_pixels;
        memset(pixels, 0, (size_t)pitch * SCREEN_H);  // fully transparent

        for (int i = 0; i < STARS_PER_LAYER; i++) {
            int sx = (int)(randf() * SCREEN_W);
            int sy = (int)(randf() * SCREEN_H);
            stamp_star(pixels, pitch_pixels, sx, sy, sizes[l], cr, cg, cb);
        }

        SDL_UnlockTexture(layers[l].texture);
    }

    start_counter = SDL_GetPerformanceCounter();
    initialized = true;
}

void background_shutdown(void) {
    for (int l = 0; l < NUM_LAYERS; l++) {
        if (layers[l].texture) {
            SDL_DestroyTexture(layers[l].texture);
            layers[l].texture = NULL;
        }
    }
    initialized = false;
}

void render_background(SDL_Renderer *renderer, f32 dt) {
    (void)dt;
    if (!initialized) background_init(renderer);

    // Compute elapsed time from absolute clock — no dt accumulation jitter
    f32 elapsed = (f32)(SDL_GetPerformanceCounter() - start_counter)
                / (f32)SDL_GetPerformanceFrequency();

    for (int l = 0; l < NUM_LAYERS; l++) {
        StarLayer *layer = &layers[l];

        // Scroll position derived from absolute time (wraps seamlessly)
        f32 offset = fmodf(elapsed * layer->speed, (f32)SCREEN_H);

        // Draw the texture twice to wrap seamlessly:
        // Top portion: the part that has scrolled down
        SDL_FRect dst_top = { 0, offset, (f32)SCREEN_W, (f32)SCREEN_H };
        SDL_RenderTexture(renderer, layer->texture, NULL, &dst_top);

        // Wrapped copy above it
        SDL_FRect dst_wrap = { 0, offset - (f32)SCREEN_H, (f32)SCREEN_W, (f32)SCREEN_H };
        SDL_RenderTexture(renderer, layer->texture, NULL, &dst_wrap);
    }
}

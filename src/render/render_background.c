#include "render/render_background.h"
#include <stdlib.h>

#define WINDOW_SIZE 800

#define NUM_LAYERS 3
#define STARS_PER_LAYER 80

typedef struct {
    f32 x, y;
} Star;

typedef struct {
    Star stars[STARS_PER_LAYER];
    f32  speed;    // pixels per second (vertical scroll)
    f32  size;     // point size in pixels
    u8   bright;   // base brightness 0-255
} StarLayer;

static StarLayer layers[NUM_LAYERS];
static bool initialized = false;

static f32 randf(void) {
    return (f32)rand() / (f32)RAND_MAX;
}

void background_init(void) {
    // Layer 0: far stars  – small, dim, slow
    // Layer 1: mid stars  – medium
    // Layer 2: near stars – large, bright, fast
    const f32  speeds[]  = { 15.0f,  35.0f,  70.0f };
    const f32  sizes[]   = {  1.0f,   2.0f,   3.0f };
    const u8   brights[] = {    80,    140,    220 };

    for (int l = 0; l < NUM_LAYERS; l++) {
        layers[l].speed  = speeds[l];
        layers[l].size   = sizes[l];
        layers[l].bright = brights[l];

        for (int i = 0; i < STARS_PER_LAYER; i++) {
            layers[l].stars[i].x = randf() * WINDOW_SIZE;
            layers[l].stars[i].y = randf() * WINDOW_SIZE;
        }
    }
    initialized = true;
}

void render_background(SDL_Renderer *renderer, f32 dt) {
    if (!initialized) background_init();

    for (int l = 0; l < NUM_LAYERS; l++) {
        StarLayer *layer = &layers[l];
        u8 b = layer->bright;

        // Slight color tint: far stars bluer, near stars whiter
        u8 r = (u8)(b * (0.7f + 0.3f * ((f32)l / (NUM_LAYERS - 1))));
        u8 g = (u8)(b * (0.8f + 0.2f * ((f32)l / (NUM_LAYERS - 1))));

        SDL_SetRenderDrawColor(renderer, r, g, b, 255);

        for (int i = 0; i < STARS_PER_LAYER; i++) {
            Star *s = &layer->stars[i];

            // Scroll downward (simulates ship moving upward)
            s->y += layer->speed * dt;

            // Wrap around
            if (s->y > WINDOW_SIZE) {
                s->y -= WINDOW_SIZE;
                s->x = randf() * WINDOW_SIZE;
            }

            // Draw star as a filled rect (size varies by layer)
            SDL_FRect rect = {
                s->x - layer->size * 0.5f,
                s->y - layer->size * 0.5f,
                layer->size,
                layer->size
            };
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

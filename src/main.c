#include <SDL3/SDL_init.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "imgui_sdl3.h"
#include "utils/q_util.h"

#include "game/game.h"
#include "render/render.h"
#include "render/planet_gen.h"

#define WINDOW_W 1280
#define WINDOW_H 720

#define NUM_LEVELS 3

static const char *level_paths[NUM_LEVELS] = {
    "assets/levels/slingshot_01.json",
    "assets/levels/gauntlet_02.json",
    "assets/levels/quad_03.json",

};

static const char *level_names[NUM_LEVELS] = {
    "Slingshot 01",
    "Gauntlet 02",
    "QuadRun 03",
};

// Frame timing breakdown (smoothed, in ms)
typedef struct {
    f32 physics;
    f32 render;
    f32 imgui;
    f32 present;
    f32 total;
} FrameTiming;

// AppState
typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  u64 last_counter;
  Game game;
  int level_idx;
  f32 fps_smooth;  // exponentially smoothed FPS
  bool show_stars;
  FrameTiming timing;
} AppState;

static inline f32 elapsed_ms(u64 start, u64 freq) {
    return (f32)(SDL_GetPerformanceCounter() - start) / (f32)freq * 1000.0f;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    AppState *state = SDL_calloc(1, sizeof(AppState));

    if (!state)
        return SDL_APP_FAILURE;
    *appstate = state;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->window = SDL_CreateWindow("GravityBoost", WINDOW_W, WINDOW_H, 0);
    if (!state->window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->renderer = SDL_CreateRenderer(state->window, NULL);
    if (!state->renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
#ifdef __EMSCRIPTEN__
    // Disable SDL vsync stall — rAF already provides frame pacing
    SDL_SetRenderVSync(state->renderer, 0);
    // Ensure the main loop uses requestAnimationFrame (not setTimeout)
    SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "0");
#else
    SDL_SetRenderVSync(state->renderer, 1);
#endif

    if (!ImGui_SDL3_Init(state->window, state->renderer)) {
        SDL_Log("ImGui_SDL3_Init failed");
        return SDL_APP_FAILURE;
    }

    state->texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_ABGR8888,
                                     SDL_TEXTUREACCESS_STREAMING, WINDOW_W, WINDOW_H);
    if (!state->texture) {
        SDL_Log("SDL_CreateTexture failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetTextureScaleMode(state->texture, SDL_SCALEMODE_NEAREST);

    state->last_counter = SDL_GetPerformanceCounter();
    state->level_idx = 0;
    state->show_stars = true;

    // Init game state (creates Box2D world + bodies)
    if (!game_init(&state->game, level_paths[state->level_idx])) {
        SDL_Log("game_init failed");
        return SDL_APP_FAILURE;
    }
    planet_textures_generate(state->renderer, &state->game);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    AppState *state = appstate;

    ImGui_SDL3_ProcessEvent(event);

    bool imgui_wants_mouse = igGetIO_Nil()->WantCaptureMouse;

    switch (event->type) {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;

    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_ESCAPE)
            return SDL_APP_SUCCESS;
        // R to reset to aim state
        if (event->key.key == SDLK_R) {
            planet_textures_destroy(&state->game);
            game_shutdown(&state->game);
            game_init(&state->game, level_paths[state->level_idx]);
            planet_textures_generate(state->renderer, &state->game);
        }
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (!imgui_wants_mouse && event->button.button == SDL_BUTTON_LEFT) {
            game_aim_start(&state->game, event->button.x, event->button.y);
        }
        break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            game_aim_release(&state->game, event->button.x, event->button.y);
        }
        break;

    case SDL_EVENT_MOUSE_MOTION:
        game_aim_move(&state->game, event->motion.x, event->motion.y);
        break;

    case SDL_EVENT_MOUSE_WHEEL:
        if (!imgui_wants_mouse) {
            Camera *cam = &state->game.cam;
            float factor = (event->wheel.y > 0) ? 1.1f : 1.0f / 1.1f;
            cam->ppm *= factor;
            if (cam->ppm < 5.0f)  cam->ppm = 5.0f;
            if (cam->ppm > 200.0f) cam->ppm = 200.0f;
        }
        break;

    default:
        break;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    AppState *state = appstate;

    // Timing (high-precision counter for smooth dt)
    u64 now = SDL_GetPerformanceCounter();
    u64 freq = SDL_GetPerformanceFrequency();
    f32 dt = (f32)(now - state->last_counter) / (f32)freq;
    state->last_counter = now;

    // Clamp dt to avoid physics explosions from lag spikes
    if (dt > 0.05f) dt = 0.05f;  // cap at 20 FPS minimum step

    // Smooth FPS (exponential moving average, ~0.5s window)
    f32 inst_fps = (dt > 0.0001f) ? 1.0f / dt : 0.0f;
    if (state->fps_smooth < 1.0f)
        state->fps_smooth = inst_fps;  // first frame init
    else
        state->fps_smooth += (inst_fps - state->fps_smooth) * 0.05f;

    u64 t0, t1, t2, t3, t4;
    f32 smooth = 0.05f;  // EMA smoothing factor

    // --- Physics ---
    t0 = SDL_GetPerformanceCounter();
    game_update(&state->game, dt);
    t1 = SDL_GetPerformanceCounter();

    // --- Render ---
    SDL_SetRenderDrawColor(state->renderer, 10, 10, 18, 255);
    SDL_RenderClear(state->renderer);

    if (state->show_stars)
        render_background(state->renderer, dt);
    render_bounds(state->renderer, &state->game);
    render_planets(state->renderer, &state->game);
    if (state->game.show_field)
        render_gravity_field(state->renderer, &state->game);
    render_ship(state->renderer, &state->game);
    t2 = SDL_GetPerformanceCounter();

    // --- ImGui ---
    ImGui_SDL3_NewFrame();

    igSetNextWindowPos((ImVec2){10, 10}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
    igSetNextWindowSize((ImVec2){250, 260}, ImGuiCond_FirstUseEver);
    igBegin("GravityBoost", NULL, 0);
    igText("FPS: %.1f", state->fps_smooth);
    igSeparator();

    int prev_idx = state->level_idx;
    igCombo_Str_arr("Level", &state->level_idx, level_names, NUM_LEVELS, -1);
    if (state->level_idx != prev_idx) {
        planet_textures_destroy(&state->game);
        game_shutdown(&state->game);
        game_init(&state->game, level_paths[state->level_idx]);
        planet_textures_generate(state->renderer, &state->game);
    }

    igCheckbox("Stars", &state->show_stars);
    igCheckbox("Gravity Field", &state->game.show_field);
    igSeparator();
    igText("Fleet: %d/%d alive", state->game.alive_count, state->game.fleet_count);
    igText("Arrived: %d/%d required", state->game.arrived_count, state->game.required_ships);

    // Frame timing breakdown
    igSeparator();
    igText("-- Frame Timing (ms) --");
    igText("Physics:  %.2f", state->timing.physics);
    igText("Render:   %.2f", state->timing.render);
    igText("ImGui:    %.2f", state->timing.imgui);
    igText("Present:  %.2f", state->timing.present);
    igText("Total:    %.2f", state->timing.total);

    igEnd();

    // --- Present ---
    ImGui_SDL3_Render(state->renderer);
    t3 = SDL_GetPerformanceCounter();
    SDL_RenderPresent(state->renderer);
    t4 = SDL_GetPerformanceCounter();

    // Update smoothed timings
    f32 ms_phys    = (f32)(t1 - t0) / (f32)freq * 1000.0f;
    f32 ms_render  = (f32)(t2 - t1) / (f32)freq * 1000.0f;
    f32 ms_imgui   = (f32)(t3 - t2) / (f32)freq * 1000.0f;
    f32 ms_present = (f32)(t4 - t3) / (f32)freq * 1000.0f;
    f32 ms_total   = (f32)(t4 - t0) / (f32)freq * 1000.0f;

    state->timing.physics  += (ms_phys    - state->timing.physics)  * smooth;
    state->timing.render   += (ms_render  - state->timing.render)   * smooth;
    state->timing.imgui    += (ms_imgui   - state->timing.imgui)    * smooth;
    state->timing.present  += (ms_present - state->timing.present)  * smooth;
    state->timing.total    += (ms_total   - state->timing.total)    * smooth;

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    (void)result;
    AppState *state = appstate;
    if (!state) return;

    background_shutdown();
    planet_textures_destroy(&state->game);
    game_shutdown(&state->game);
    ImGui_SDL3_Shutdown();

    if (state->texture)  SDL_DestroyTexture(state->texture);
    if (state->renderer) SDL_DestroyRenderer(state->renderer);
    if (state->window)   SDL_DestroyWindow(state->window);

    SDL_free(state);
}

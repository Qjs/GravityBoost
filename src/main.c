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

#define NUM_LEVELS 2

static const char *level_paths[NUM_LEVELS] = {
    "assets/levels/slingshot_01.json",
    "assets/levels/gauntlet_02.json",
};

static const char *level_names[NUM_LEVELS] = {
    "Slingshot 01",
    "Gauntlet 02",
};

// AppState
typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  u64 last_ticks;
  Game game;
  int level_idx;
} AppState;

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

    state->last_ticks = SDL_GetTicks();
    state->level_idx = 0;

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

    // Timing
    u64 now = SDL_GetTicks();
    f32 dt = (f32)(now - state->last_ticks) / 1000.0f;
    state->last_ticks = now;

    // Update game (Box2D steps inside)
    game_update(&state->game, dt);

    // Clear
    SDL_SetRenderDrawColor(state->renderer, 10, 10, 18, 255);
    SDL_RenderClear(state->renderer);

    // Parallax starfield
    render_background(state->renderer, dt);

    // Level bounds
    render_bounds(state->renderer, &state->game);

    // Game objects
    render_planets(state->renderer, &state->game);
    if (state->game.show_field)
        render_gravity_field(state->renderer, &state->game);
    render_ship(state->renderer, &state->game);

    // ImGui frame
    ImGui_SDL3_NewFrame();

    igSetNextWindowPos((ImVec2){10, 10}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
    igSetNextWindowSize((ImVec2){250, 180}, ImGuiCond_FirstUseEver);
    igBegin("GravityBoost", NULL, 0);
    igText("FPS: %.1f", 1.0f / (dt > 0.0001f ? dt : 0.016f));
    igSeparator();

    int prev_idx = state->level_idx;
    igCombo_Str_arr("Level", &state->level_idx, level_names, NUM_LEVELS, -1);
    if (state->level_idx != prev_idx) {
        planet_textures_destroy(&state->game);
        game_shutdown(&state->game);
        game_init(&state->game, level_paths[state->level_idx]);
        planet_textures_generate(state->renderer, &state->game);
    }

    igCheckbox("Gravity Field", &state->game.show_field);
    igSeparator();
    igText("Fleet: %d/%d alive", state->game.alive_count, state->game.fleet_count);
    igText("Arrived: %d/%d required", state->game.arrived_count, state->game.required_ships);

    igEnd();

    // Render
    ImGui_SDL3_Render(state->renderer);
    SDL_RenderPresent(state->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    (void)result;
    AppState *state = appstate;
    if (!state) return;

    planet_textures_destroy(&state->game);
    game_shutdown(&state->game);
    ImGui_SDL3_Shutdown();

    if (state->texture)  SDL_DestroyTexture(state->texture);
    if (state->renderer) SDL_DestroyRenderer(state->renderer);
    if (state->window)   SDL_DestroyWindow(state->window);

    SDL_free(state);
}

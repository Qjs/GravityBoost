#include <SDL3/SDL_init.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "imgui_sdl3.h"
#include "utils/q_util.h"

#include <box2d/box2d.h>
#include "cJSON.h"
#include "game/game.h"
#include "render/render.h"

#define WINDOW_W 1280
#define WINDOW_H 720

// AppState
typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  u64 last_ticks;
  b2WorldId world_id;
  Game game;
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

    // Create Box2D world (proves Box2D links)
    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity = (b2Vec2){0.0f, -9.81f};
    state->world_id = b2CreateWorld(&world_def);
    SDL_Log("Box2D world created");

    // Test cJSON (proves cJSON links)
    cJSON *json = cJSON_Parse("{\"game\":\"GravityBoost\"}");
    if (json) {
        cJSON *item = cJSON_GetObjectItem(json, "game");
        if (cJSON_IsString(item)) {
            SDL_Log("cJSON OK: game = %s", item->valuestring);
        }
        cJSON_Delete(json);
    }

    // Init game state with test level
    if (!game_init(&state->game)) {
        SDL_Log("game_init failed");
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    (void)appstate;

    ImGui_SDL3_ProcessEvent(event);

    bool imgui_wants_mouse = igGetIO_Nil()->WantCaptureMouse;

    switch (event->type) {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;

    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_ESCAPE)
            return SDL_APP_SUCCESS;
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (!imgui_wants_mouse) {
            if (event->button.button == SDL_BUTTON_LEFT) {
            }
            if (event->button.button == SDL_BUTTON_RIGHT) {
            }
        }
        break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
        break;

    case SDL_EVENT_MOUSE_MOTION:
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

    // Step Box2D world
    b2World_Step(state->world_id, dt, 4);

    // Clear
    SDL_SetRenderDrawColor(state->renderer, 10, 10, 18, 255);
    SDL_RenderClear(state->renderer);

    // Parallax starfield
    render_background(state->renderer, dt);

    // Game objects
    render_planets(state->renderer, &state->game);
    render_ship(state->renderer, &state->game);

    // ImGui frame
    ImGui_SDL3_NewFrame();

    igSetNextWindowPos((ImVec2){10, 10}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
    igSetNextWindowSize((ImVec2){250, 150}, ImGuiCond_FirstUseEver);
    igBegin("GravityBoost", NULL, 0);
    igText("FPS: %.1f", 1.0f / (dt > 0.0001f ? dt : 0.016f));
    igSeparator();

    static int level_idx = 0;
    const char *levels[] = {"Level 1", "Level 2", "Level 3"};
    igCombo_Str_arr("Level", &level_idx, levels, 3, -1);

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

    game_shutdown(&state->game);
    b2DestroyWorld(state->world_id);
    ImGui_SDL3_Shutdown();

    if (state->texture)  SDL_DestroyTexture(state->texture);
    if (state->renderer) SDL_DestroyRenderer(state->renderer);
    if (state->window)   SDL_DestroyWindow(state->window);

    SDL_free(state);
}

#include <SDL3/SDL_init.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "imgui_sdl3.h"
#include "utils/q_util.h"

// game specific includes

#define WINDOW_SIZE 800

// AppState
typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  u64 last_ticks;
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

    state->window = SDL_CreateWindow("GravityBoost", WINDOW_SIZE, WINDOW_SIZE, 0);
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

    s32 grid = WINDOW_SIZE;
    state->texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_ABGR8888,
                                     SDL_TEXTUREACCESS_STREAMING, grid, grid);
    if (!state->texture) {
        SDL_Log("SDL_CreateTexture failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetTextureScaleMode(state->texture, SDL_SCALEMODE_NEAREST);

    state->last_ticks = SDL_GetTicks();

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
    // if (event->key.key == SDLK_R && !event->key.repeat) {

    //   }
        break;

  // Mouse Motion
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (!imgui_wants_mouse) {
            if (event->button.button == SDL_BUTTON_LEFT) {
            // Left mouse
            }
            if (event->button.button == SDL_BUTTON_RIGHT) {
            // Right Mouse
            }
        }
        break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
        }

        if (event->button.button == SDL_BUTTON_RIGHT) {
        }
        break;

    case SDL_EVENT_MOUSE_MOTION:
        if (!imgui_wants_mouse) {
        // int prev_x = state->input.mouse_x;
        // int prev_y = state->input.mouse_y;
        }
        break;

    default:
        break;
    }
}

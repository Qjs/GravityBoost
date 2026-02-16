#include <SDL3/SDL_init.h>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "imgui_sdl3.h"

#include "editor/editor_state.h"
#include "editor/editor_ui.h"
#include "editor/editor_interact.h"
#include "editor/editor_save.h"
#include "render/render_background.h"
#include "render/render_bounds.h"
#include "render/render_planets.h"
#include "render/render_field.h"
#include "render/planet_gen.h"

#include <math.h>

#define WINDOW_W 1280
#define WINDOW_H 720

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    u64           last_ticks;
    EditorState   es;
} EditorApp;

// -----------------------------------------------------------------------
// Drawing helpers
// -----------------------------------------------------------------------

static void draw_circle_outline(SDL_Renderer *renderer,
                                f32 cx, f32 cy, f32 r, s32 segments) {
    f32 step = 2.0f * (f32)M_PI / segments;
    f32 px = cx + r;
    f32 py = cy;
    for (s32 i = 1; i <= segments; i++) {
        f32 a = step * i;
        f32 nx = cx + r * cosf(a);
        f32 ny = cy + r * sinf(a);
        SDL_RenderLine(renderer, px, py, nx, ny);
        px = nx;
        py = ny;
    }
}

static void draw_filled_circle(SDL_Renderer *renderer,
                                f32 cx, f32 cy, f32 r) {
    for (f32 dy = -r; dy <= r; dy += 1.0f) {
        f32 dx = sqrtf(r * r - dy * dy);
        SDL_RenderLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

// -----------------------------------------------------------------------
// SDL3 app callbacks
// -----------------------------------------------------------------------

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
    (void)argc; (void)argv;

    EditorApp *app = SDL_calloc(1, sizeof(EditorApp));
    if (!app) return SDL_APP_FAILURE;
    *appstate = app;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    app->window = SDL_CreateWindow("GravityBoost Editor", WINDOW_W, WINDOW_H, 0);
    if (!app->window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    app->renderer = SDL_CreateRenderer(app->window, NULL);
    if (!app->renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!ImGui_SDL3_Init(app->window, app->renderer)) {
        SDL_Log("ImGui_SDL3_Init failed");
        return SDL_APP_FAILURE;
    }

    app->last_ticks = SDL_GetTicks();

    editor_state_defaults(&app->es);

    // If a level path was passed on the command line, load it
    if (argc > 1) {
        snprintf(app->es.file_path, sizeof(app->es.file_path), "%s", argv[1]);
        editor_load(&app->es, argv[1], app->renderer);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    EditorApp *app = appstate;

    ImGui_SDL3_ProcessEvent(event);

    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;

    // Let ImGui have priority
    bool imgui_mouse = igGetIO_Nil()->WantCaptureMouse;
    bool imgui_kb    = igGetIO_Nil()->WantCaptureKeyboard;

    bool is_mouse = (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                     event->type == SDL_EVENT_MOUSE_BUTTON_UP   ||
                     event->type == SDL_EVENT_MOUSE_MOTION      ||
                     event->type == SDL_EVENT_MOUSE_WHEEL);
    bool is_kb    = (event->type == SDL_EVENT_KEY_DOWN ||
                     event->type == SDL_EVENT_KEY_UP);

    if ((is_mouse && !imgui_mouse) || (is_kb && !imgui_kb))
        editor_handle_event(&app->es, event, app->renderer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    EditorApp *app = appstate;
    EditorState *es = &app->es;

    // Timing
    u64 now = SDL_GetTicks();
    f32 dt  = (f32)(now - app->last_ticks) / 1000.0f;
    app->last_ticks = now;

    // Regenerate planet textures if something changed
    if (es->textures_dirty) {
        planet_textures_destroy(&es->game);
        planet_textures_generate(app->renderer, &es->game);
        es->textures_dirty = false;
    }

    // Spin planet textures
    for (s32 i = 0; i < es->game.planet_count; i++) {
        es->game.planets[i].rotation_angle += es->game.planets[i].rotation_speed * dt;
        if (es->game.planets[i].rotation_angle >= 360.0f)
            es->game.planets[i].rotation_angle -= 360.0f;
    }

    // ---- Render ----
    SDL_SetRenderDrawColor(app->renderer, 10, 10, 18, 255);
    SDL_RenderClear(app->renderer);

    render_background(app->renderer, dt);
    render_bounds(app->renderer, &es->game);
    render_planets(app->renderer, &es->game);

    if (es->game.show_field)
        render_gravity_field(app->renderer, &es->game);

    // Start marker (blue filled circle)
    {
        const Camera *cam = &es->game.cam;
        f32 sx = world_to_screen_x(cam, es->game.ships[0].pos.x);
        f32 sy = world_to_screen_y(cam, es->game.ships[0].pos.y);

        SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(app->renderer, 100, 180, 255, 200);
        draw_filled_circle(app->renderer, sx, sy, 8.0f);
        SDL_SetRenderDrawColor(app->renderer, 200, 220, 255, 255);
        draw_circle_outline(app->renderer, sx, sy, 10.0f, 24);
    }

    // Selection highlight (yellow ring)
    if (es->selected != SEL_NONE) {
        const Camera *cam = &es->game.cam;
        f32 sx = 0, sy = 0, sr = 0;
        bool valid = false;

        if (es->selected >= 0 && es->selected < es->game.planet_count) {
            const Planet *p = &es->game.planets[es->selected];
            sx = world_to_screen_x(cam, p->pos.x);
            sy = world_to_screen_y(cam, p->pos.y);
            sr = world_to_screen_r(cam, p->radius) + 6.0f;
            valid = true;
        } else if (es->selected == SEL_GOAL) {
            sx = world_to_screen_x(cam, es->game.goal.pos.x);
            sy = world_to_screen_y(cam, es->game.goal.pos.y);
            sr = world_to_screen_r(cam, es->game.goal.radius) + 6.0f;
            valid = true;
        } else if (es->selected == SEL_START) {
            sx = world_to_screen_x(cam, es->game.ships[0].pos.x);
            sy = world_to_screen_y(cam, es->game.ships[0].pos.y);
            sr = 14.0f;
            valid = true;
        }

        if (valid) {
            SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(app->renderer, 255, 255, 0, 180);
            draw_circle_outline(app->renderer, sx, sy, sr, 48);
            SDL_SetRenderDrawColor(app->renderer, 255, 255, 0, 80);
            draw_circle_outline(app->renderer, sx, sy, sr + 2.0f, 48);
        }
    }

    // Hover highlight (white ring, only if different from selection)
    if (es->hovered != SEL_NONE && es->hovered != es->selected) {
        const Camera *cam = &es->game.cam;
        f32 sx = 0, sy = 0, sr = 0;
        bool valid = false;

        if (es->hovered >= 0 && es->hovered < es->game.planet_count) {
            const Planet *p = &es->game.planets[es->hovered];
            sx = world_to_screen_x(cam, p->pos.x);
            sy = world_to_screen_y(cam, p->pos.y);
            sr = world_to_screen_r(cam, p->radius) + 4.0f;
            valid = true;
        } else if (es->hovered == SEL_GOAL) {
            sx = world_to_screen_x(cam, es->game.goal.pos.x);
            sy = world_to_screen_y(cam, es->game.goal.pos.y);
            sr = world_to_screen_r(cam, es->game.goal.radius) + 4.0f;
            valid = true;
        } else if (es->hovered == SEL_START) {
            sx = world_to_screen_x(cam, es->game.ships[0].pos.x);
            sy = world_to_screen_y(cam, es->game.ships[0].pos.y);
            sr = 12.0f;
            valid = true;
        }

        if (valid) {
            SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(app->renderer, 255, 255, 255, 80);
            draw_circle_outline(app->renderer, sx, sy, sr, 48);
        }
    }

    SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_NONE);

    // ImGui
    ImGui_SDL3_NewFrame();
    editor_ui(es, app->renderer);
    ImGui_SDL3_Render(app->renderer);

    SDL_RenderPresent(app->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    (void)result;
    EditorApp *app = appstate;
    if (!app) return;

    planet_textures_destroy(&app->es.game);
    ImGui_SDL3_Shutdown();

    if (app->renderer) SDL_DestroyRenderer(app->renderer);
    if (app->window)   SDL_DestroyWindow(app->window);

    SDL_free(app);
}

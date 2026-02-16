#include "editor/editor_interact.h"
#include "render/planet_gen.h"
#include <SDL3/SDL.h>
#include <math.h>

#define HIT_MARGIN 0.5f  // extra world-space hit tolerance

static s32 hit_test(const EditorState *es, f32 wx, f32 wy) {
    const Game *g = &es->game;

    // Planets (back-to-front, topmost first)
    for (s32 i = g->planet_count - 1; i >= 0; i--) {
        f32 dx = wx - g->planets[i].pos.x;
        f32 dy = wy - g->planets[i].pos.y;
        if (sqrtf(dx * dx + dy * dy) <= g->planets[i].radius + HIT_MARGIN)
            return i;
    }

    // Goal
    {
        f32 dx = wx - g->goal.pos.x;
        f32 dy = wy - g->goal.pos.y;
        if (sqrtf(dx * dx + dy * dy) <= g->goal.radius + HIT_MARGIN)
            return SEL_GOAL;
    }

    // Start marker (treat as ~0.5 world-unit radius)
    {
        f32 dx = wx - g->ships[0].pos.x;
        f32 dy = wy - g->ships[0].pos.y;
        if (sqrtf(dx * dx + dy * dy) <= 0.5f + HIT_MARGIN)
            return SEL_START;
    }

    return SEL_NONE;
}

void editor_handle_event(EditorState *es, const SDL_Event *event,
                         SDL_Renderer *renderer) {
    Camera *cam = &es->game.cam;
    (void)renderer;

    switch (event->type) {

    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        f32 wx = screen_to_world_x(cam, event->button.x);
        f32 wy = screen_to_world_y(cam, event->button.y);

        if (event->button.button == SDL_BUTTON_LEFT) {
            if (es->adding_planet) {
                if (es->game.planet_count < MAX_PLANETS) {
                    s32 idx = es->game.planet_count++;
                    Planet *p = &es->game.planets[idx];
                    *p = (Planet){0};
                    p->pos    = (Vec2){ wx, wy };
                    p->radius = 2.0f;
                    p->mu     = 50.0f;
                    p->eps    = 0.5f;
                    // seed from position
                    u32 hx = *(u32 *)&p->pos.x;
                    u32 hy = *(u32 *)&p->pos.y;
                    p->seed = hx * 2654435761u ^ hy * 2246822519u ^ (u32)idx;
                    p->type = (PlanetType)(p->seed % PLANET_TYPE_COUNT);
                    f32 base_speeds[] = { 2.0f, 3.0f, 8.0f, 1.5f, 5.0f };
                    p->rotation_speed = base_speeds[p->type];
                    es->textures_dirty = true;
                    es->selected       = idx;
                    es->adding_planet  = false;
                }
            } else {
                s32 hit = hit_test(es, wx, wy);
                es->selected = hit;
                if (hit != SEL_NONE) {
                    es->dragging = true;
                    Vec2 obj_pos;
                    if (hit >= 0 && hit < es->game.planet_count)
                        obj_pos = es->game.planets[hit].pos;
                    else if (hit == SEL_GOAL)
                        obj_pos = es->game.goal.pos;
                    else
                        obj_pos = es->game.ships[0].pos;
                    es->drag_offset = (Vec2){
                        obj_pos.x - wx,
                        obj_pos.y - wy
                    };
                }
            }
        }
        break;
    }

    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT)
            es->dragging = false;
        break;

    case SDL_EVENT_MOUSE_MOTION:
        if (event->motion.state & SDL_BUTTON_MMASK) {
            // Camera pan via middle-mouse drag
            cam->cam_x -= event->motion.xrel / cam->ppm;
            cam->cam_y += event->motion.yrel / cam->ppm;
        } else if (es->dragging && (event->motion.state & SDL_BUTTON_LMASK)) {
            f32 wx = screen_to_world_x(cam, event->motion.x);
            f32 wy = screen_to_world_y(cam, event->motion.y);
            f32 nx = wx + es->drag_offset.x;
            f32 ny = wy + es->drag_offset.y;

            if (es->selected >= 0 && es->selected < es->game.planet_count) {
                es->game.planets[es->selected].pos = (Vec2){ nx, ny };
            } else if (es->selected == SEL_GOAL) {
                es->game.goal.pos = (Vec2){ nx, ny };
            } else if (es->selected == SEL_START) {
                es->game.ships[0].pos = (Vec2){ nx, ny };
            }
        } else {
            // Hover highlight
            f32 wx = screen_to_world_x(cam, event->motion.x);
            f32 wy = screen_to_world_y(cam, event->motion.y);
            es->hovered = hit_test(es, wx, wy);
        }
        break;

    case SDL_EVENT_MOUSE_WHEEL: {
        float factor = (event->wheel.y > 0) ? 1.1f : 1.0f / 1.1f;
        cam->ppm *= factor;
        if (cam->ppm < 5.0f)   cam->ppm = 5.0f;
        if (cam->ppm > 200.0f) cam->ppm = 200.0f;
        break;
    }

    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_DELETE || event->key.key == SDLK_BACKSPACE) {
            if (es->selected >= 0 && es->selected < es->game.planet_count) {
                if (es->game.planets[es->selected].texture)
                    SDL_DestroyTexture(es->game.planets[es->selected].texture);
                for (s32 i = es->selected; i < es->game.planet_count - 1; i++)
                    es->game.planets[i] = es->game.planets[i + 1];
                es->game.planet_count--;
                es->game.planets[es->game.planet_count].texture = NULL;
                es->selected = SEL_NONE;
            }
        }
        if (event->key.key == SDLK_ESCAPE) {
            es->adding_planet = false;
            es->selected = SEL_NONE;
        }
        break;

    default:
        break;
    }
}

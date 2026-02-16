#pragma once

#include "editor/editor_state.h"

struct SDL_Renderer;
union  SDL_Event;

void editor_handle_event(EditorState *es, const union SDL_Event *event,
                         struct SDL_Renderer *renderer);

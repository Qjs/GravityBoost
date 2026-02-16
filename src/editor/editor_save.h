#pragma once

#include "editor/editor_state.h"

struct SDL_Renderer;

bool editor_save(const EditorState *es, const char *path);
bool editor_load(EditorState *es, const char *path, struct SDL_Renderer *renderer);

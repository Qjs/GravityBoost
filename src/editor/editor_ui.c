#include "editor/editor_ui.h"
#include "editor/editor_save.h"
#include "render/planet_gen.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include <SDL3/SDL.h>

static const char *planet_type_names[] = {
    "Rocky", "Terrestrial", "Gas Giant", "Ice", "Volcanic"
};

void editor_ui(EditorState *es, SDL_Renderer *renderer) {
    // ------------------------------------------------------------------
    // Toolbar
    // ------------------------------------------------------------------
    igSetNextWindowPos((ImVec2){10, 10}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
    igSetNextWindowSize((ImVec2){200, 130}, ImGuiCond_FirstUseEver);
    igBegin("Toolbar", NULL, 0);

    if (igButton("Add Planet", (ImVec2){-1, 0})) {
        es->adding_planet = !es->adding_planet;
        if (es->adding_planet) es->selected = SEL_NONE;
    }
    if (es->adding_planet)
        igTextColored((ImVec4){1, 1, 0, 1}, "Click canvas to place");

    bool can_delete = (es->selected >= 0 && es->selected < es->game.planet_count);
    if (!can_delete) igBeginDisabled(true);
    if (igButton("Delete Selected", (ImVec2){-1, 0}) && can_delete) {
        Planet *p = &es->game.planets[es->selected];
        if (p->texture) SDL_DestroyTexture(p->texture);
        for (s32 i = es->selected; i < es->game.planet_count - 1; i++)
            es->game.planets[i] = es->game.planets[i + 1];
        es->game.planet_count--;
        es->game.planets[es->game.planet_count].texture = NULL;
        es->selected = SEL_NONE;
    }
    if (!can_delete) igEndDisabled();

    igCheckbox("Gravity Field", &es->game.show_field);

    igEnd();

    // ------------------------------------------------------------------
    // File
    // ------------------------------------------------------------------
    igSetNextWindowPos((ImVec2){10, 150}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
    igSetNextWindowSize((ImVec2){200, 100}, ImGuiCond_FirstUseEver);
    igBegin("File", NULL, 0);

    igInputText("Path", es->file_path, sizeof(es->file_path), 0, NULL, NULL);

    if (igButton("Load", (ImVec2){90, 0}))
        editor_load(es, es->file_path, renderer);
    igSameLine(0, 10);
    if (igButton("Save", (ImVec2){90, 0}) && es->file_path[0])
        editor_save(es, es->file_path);

    igEnd();

    // ------------------------------------------------------------------
    // Level Settings
    // ------------------------------------------------------------------
    igSetNextWindowPos((ImVec2){1280 - 310, 10}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
    igSetNextWindowSize((ImVec2){300, 520}, ImGuiCond_FirstUseEver);
    igBegin("Level Settings", NULL, 0);

    igInputText("Name", es->name, sizeof(es->name), 0, NULL, NULL);
    igSeparator();

    igText("Bounds");
    igDragFloat2("Min", (float *)&es->game.bounds_min, 0.5f, -100, 100, "%.1f", 0);
    igDragFloat2("Max", (float *)&es->game.bounds_max, 0.5f, -100, 100, "%.1f", 0);
    igSeparator();

    igText("Start");
    igDragFloat2("Start Pos", (float *)&es->game.ships[0].pos, 0.1f, -100, 100, "%.2f", 0);
    igDragFloat("Vel Max", &es->game.vel_max, 0.5f, 1.0f, 100.0f, "%.1f", 0);
    igSeparator();

    igText("Goal");
    igDragFloat2("Goal Pos", (float *)&es->game.goal.pos, 0.1f, -100, 100, "%.2f", 0);
    igDragFloat("Goal Radius", &es->game.goal.radius, 0.05f, 0.1f, 10.0f, "%.2f", 0);
    igSeparator();

    igText("Ship");
    igDragFloat("Ship Radius", &es->game.ships[0].radius, 0.01f, 0.05f, 5.0f, "%.2f", 0);
    igDragFloat("Density", &es->ship_density, 0.1f, 0.1f, 10.0f, "%.1f", 0);
    igDragFloat("Restitution", &es->ship_restitution, 0.01f, 0.0f, 1.0f, "%.2f", 0);
    igSeparator();

    igText("Fleet");
    igDragInt("Count", &es->game.fleet_count, 1, 1, MAX_FLEET, "%d", 0);
    igDragInt("Required", &es->game.required_ships, 1, 1, MAX_FLEET, "%d", 0);
    igSeparator();

    igText("Camera");
    igDragFloat("PPM", &es->game.cam.ppm, 1.0f, 5.0f, 200.0f, "%.0f", 0);
    igSeparator();

    igText("Allow Place");
    igCheckbox("Sink", &es->allow_sink);
    igSameLine(0, 10);
    igCheckbox("Repel", &es->allow_repel);
    igDragInt("Max##allow", &es->allow_max, 1, 0, 10, "%d", 0);

    igCheckbox("Show Field Default", &es->show_field_default);

    igEnd();

    // ------------------------------------------------------------------
    // Properties (selected object)
    // ------------------------------------------------------------------
    igSetNextWindowPos((ImVec2){1280 - 310, 540}, ImGuiCond_FirstUseEver, (ImVec2){0, 0});
    igSetNextWindowSize((ImVec2){300, 170}, ImGuiCond_FirstUseEver);
    igBegin("Properties", NULL, 0);

    if (es->selected >= 0 && es->selected < es->game.planet_count) {
        Planet *p = &es->game.planets[es->selected];
        igText("Planet %d", es->selected);
        igSeparator();

        igDragFloat2("Position##p", (float *)&p->pos, 0.1f, -100, 100, "%.2f", 0);
        igDragFloat("Radius##p", &p->radius, 0.05f, 0.1f, 20.0f, "%.2f", 0);
        igDragFloat("Mu (gravity)", &p->mu, 1.0f, 0.0f, 500.0f, "%.1f", 0);
        igDragFloat("Eps (softening)", &p->eps, 0.001f, 0.0f, 5.0f, "%.3f", 0);

        int type_int = (int)p->type;
        if (igCombo_Str_arr("Type", &type_int, planet_type_names, PLANET_TYPE_COUNT, -1)) {
            p->type = (PlanetType)type_int;
            f32 base_speeds[] = { 2.0f, 3.0f, 8.0f, 1.5f, 5.0f };
            p->rotation_speed = base_speeds[p->type];
            es->textures_dirty = true;
        }

        int seed_int = (int)p->seed;
        if (igDragInt("Seed", &seed_int, 1, 0, 99999, "%d", 0)) {
            p->seed = (u32)seed_int;
            es->textures_dirty = true;
        }

        if (igButton("Regenerate Texture", (ImVec2){-1, 0}))
            es->textures_dirty = true;

    } else if (es->selected == SEL_START) {
        igText("Start Position");
        igSeparator();
        igDragFloat2("Position##s", (float *)&es->game.ships[0].pos, 0.1f, -100, 100, "%.2f", 0);
        igDragFloat("Vel Max##s", &es->game.vel_max, 0.5f, 1.0f, 100.0f, "%.1f", 0);

    } else if (es->selected == SEL_GOAL) {
        igText("Goal");
        igSeparator();
        igDragFloat2("Position##g", (float *)&es->game.goal.pos, 0.1f, -100, 100, "%.2f", 0);
        igDragFloat("Radius##g", &es->game.goal.radius, 0.05f, 0.1f, 10.0f, "%.2f", 0);

    } else {
        igTextDisabled("No object selected");
    }

    igEnd();
}

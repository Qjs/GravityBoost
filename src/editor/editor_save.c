#include "editor/editor_save.h"
#include "data/fs.h"
#include "render/planet_gen.h"
#include <SDL3/SDL.h>
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static cJSON *vec2_to_json(Vec2 v) {
    cJSON *arr = cJSON_CreateArray();
    cJSON_AddItemToArray(arr, cJSON_CreateNumber(v.x));
    cJSON_AddItemToArray(arr, cJSON_CreateNumber(v.y));
    return arr;
}

static bool parse_vec2(const cJSON *arr, Vec2 *v) {
    if (!cJSON_IsArray(arr) || cJSON_GetArraySize(arr) < 2) return false;
    v->x = (f32)cJSON_GetArrayItem(arr, 0)->valuedouble;
    v->y = (f32)cJSON_GetArrayItem(arr, 1)->valuedouble;
    return true;
}

// ---------------------------------------------------------------------------
// Save
// ---------------------------------------------------------------------------

bool editor_save(const EditorState *es, const char *path) {
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "name", es->name);
    cJSON_AddNumberToObject(root, "ppm", es->game.cam.ppm);

    // bounds
    cJSON *bounds = cJSON_AddObjectToObject(root, "bounds");
    cJSON_AddItemToObject(bounds, "min", vec2_to_json(es->game.bounds_min));
    cJSON_AddItemToObject(bounds, "max", vec2_to_json(es->game.bounds_max));

    // start
    cJSON *start = cJSON_AddObjectToObject(root, "start");
    cJSON_AddItemToObject(start, "pos", vec2_to_json(es->game.ships[0].pos));
    cJSON_AddNumberToObject(start, "vel_max", es->game.vel_max);

    // goal
    cJSON *goal = cJSON_AddObjectToObject(root, "goal");
    cJSON_AddItemToObject(goal, "pos", vec2_to_json(es->game.goal.pos));
    cJSON_AddNumberToObject(goal, "radius", es->game.goal.radius);

    // ship
    cJSON *ship = cJSON_AddObjectToObject(root, "ship");
    cJSON_AddNumberToObject(ship, "radius", es->game.ships[0].radius);
    cJSON_AddNumberToObject(ship, "density", es->ship_density);
    cJSON_AddNumberToObject(ship, "restitution", es->ship_restitution);

    // planets
    cJSON *planets = cJSON_AddArrayToObject(root, "planets");
    for (s32 i = 0; i < es->game.planet_count; i++) {
        const Planet *p = &es->game.planets[i];
        cJSON *pj = cJSON_CreateObject();
        cJSON_AddItemToObject(pj, "pos", vec2_to_json(p->pos));
        cJSON_AddNumberToObject(pj, "radius", p->radius);
        cJSON_AddNumberToObject(pj, "mu", p->mu);
        cJSON_AddNumberToObject(pj, "eps", p->eps);
        cJSON_AddNumberToObject(pj, "seed", p->seed);
        cJSON_AddNumberToObject(pj, "type", (int)p->type);
        cJSON_AddItemToArray(planets, pj);
    }

    // allow_place
    cJSON *allow = cJSON_AddObjectToObject(root, "allow_place");
    cJSON_AddBoolToObject(allow, "sink", es->allow_sink);
    cJSON_AddBoolToObject(allow, "repel", es->allow_repel);
    cJSON_AddNumberToObject(allow, "max", es->allow_max);

    // fleet
    cJSON *fleet = cJSON_AddObjectToObject(root, "fleet");
    cJSON_AddNumberToObject(fleet, "count", es->game.fleet_count);
    cJSON_AddNumberToObject(fleet, "required", es->game.required_ships);

    // ui
    cJSON *ui = cJSON_AddObjectToObject(root, "ui");
    cJSON_AddBoolToObject(ui, "show_field_default", es->show_field_default);

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    if (!json_str) return false;

    FILE *f = fopen(path, "w");
    if (!f) {
        free(json_str);
        return false;
    }
    fputs(json_str, f);
    fclose(f);
    free(json_str);

    SDL_Log("editor_save: wrote '%s'", path);
    return true;
}

// ---------------------------------------------------------------------------
// Load
// ---------------------------------------------------------------------------

bool editor_load(EditorState *es, const char *path, SDL_Renderer *renderer) {
    char *buf = NULL;
    long  size = 0;
    if (!fs_read_file(path, &buf, &size)) {
        SDL_Log("editor_load: failed to read '%s'", path);
        return false;
    }

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        SDL_Log("editor_load: parse error: %s", cJSON_GetErrorPtr());
        return false;
    }

    // Destroy old planet textures
    planet_textures_destroy(&es->game);

    // Preserve screen dimensions, then reset
    s32 sw = es->game.cam.screen_w;
    s32 sh = es->game.cam.screen_h;
    editor_state_defaults(es);
    es->game.cam.screen_w = sw;
    es->game.cam.screen_h = sh;

    // name
    cJSON *jname = cJSON_GetObjectItem(root, "name");
    if (cJSON_IsString(jname))
        snprintf(es->name, sizeof(es->name), "%s", jname->valuestring);

    // ppm
    cJSON *jppm = cJSON_GetObjectItem(root, "ppm");
    if (cJSON_IsNumber(jppm))
        es->game.cam.ppm = (f32)jppm->valuedouble;

    // bounds
    cJSON *jbounds = cJSON_GetObjectItem(root, "bounds");
    if (jbounds) {
        cJSON *bmin = cJSON_GetObjectItem(jbounds, "min");
        cJSON *bmax = cJSON_GetObjectItem(jbounds, "max");
        if (bmin) parse_vec2(bmin, &es->game.bounds_min);
        if (bmax) parse_vec2(bmax, &es->game.bounds_max);
    }

    // start
    cJSON *jstart = cJSON_GetObjectItem(root, "start");
    if (jstart) {
        cJSON *pos = cJSON_GetObjectItem(jstart, "pos");
        if (pos) parse_vec2(pos, &es->game.ships[0].pos);
        cJSON *vel_max = cJSON_GetObjectItem(jstart, "vel_max");
        if (cJSON_IsNumber(vel_max))
            es->game.vel_max = (f32)vel_max->valuedouble;
    }

    // goal
    cJSON *jgoal = cJSON_GetObjectItem(root, "goal");
    if (jgoal) {
        cJSON *pos = cJSON_GetObjectItem(jgoal, "pos");
        if (pos) parse_vec2(pos, &es->game.goal.pos);
        cJSON *radius = cJSON_GetObjectItem(jgoal, "radius");
        if (cJSON_IsNumber(radius))
            es->game.goal.radius = (f32)radius->valuedouble;
    }

    // ship
    cJSON *jship = cJSON_GetObjectItem(root, "ship");
    if (jship) {
        cJSON *radius = cJSON_GetObjectItem(jship, "radius");
        if (cJSON_IsNumber(radius))
            es->game.ships[0].radius = (f32)radius->valuedouble;
        cJSON *density = cJSON_GetObjectItem(jship, "density");
        if (cJSON_IsNumber(density))
            es->ship_density = (f32)density->valuedouble;
        cJSON *restitution = cJSON_GetObjectItem(jship, "restitution");
        if (cJSON_IsNumber(restitution))
            es->ship_restitution = (f32)restitution->valuedouble;
    }

    // planets
    cJSON *jplanets = cJSON_GetObjectItem(root, "planets");
    if (cJSON_IsArray(jplanets)) {
        int count = cJSON_GetArraySize(jplanets);
        if (count > MAX_PLANETS) count = MAX_PLANETS;
        es->game.planet_count = count;

        for (int i = 0; i < count; i++) {
            cJSON *jp = cJSON_GetArrayItem(jplanets, i);
            Planet *planet = &es->game.planets[i];

            cJSON *pos = cJSON_GetObjectItem(jp, "pos");
            if (pos) parse_vec2(pos, &planet->pos);

            cJSON *radius = cJSON_GetObjectItem(jp, "radius");
            if (cJSON_IsNumber(radius))
                planet->radius = (f32)radius->valuedouble;

            cJSON *mu = cJSON_GetObjectItem(jp, "mu");
            if (cJSON_IsNumber(mu))
                planet->mu = (f32)mu->valuedouble;

            cJSON *eps = cJSON_GetObjectItem(jp, "eps");
            if (cJSON_IsNumber(eps))
                planet->eps = (f32)eps->valuedouble;

            cJSON *seed_j = cJSON_GetObjectItem(jp, "seed");
            if (cJSON_IsNumber(seed_j)) {
                planet->seed = (u32)seed_j->valuedouble;
            } else {
                u32 hx = *(u32 *)&planet->pos.x;
                u32 hy = *(u32 *)&planet->pos.y;
                planet->seed = hx * 2654435761u ^ hy * 2246822519u ^ (u32)i;
            }

            cJSON *type_j = cJSON_GetObjectItem(jp, "type");
            if (cJSON_IsNumber(type_j)) {
                planet->type = (PlanetType)((int)type_j->valuedouble % PLANET_TYPE_COUNT);
            } else {
                planet->type = (PlanetType)(planet->seed % PLANET_TYPE_COUNT);
            }

            f32 base_speeds[] = { 2.0f, 3.0f, 8.0f, 1.5f, 5.0f };
            planet->rotation_speed = base_speeds[planet->type];
            planet->rotation_angle = 0.0f;
            planet->texture = NULL;
        }
    }

    // fleet
    cJSON *jfleet = cJSON_GetObjectItem(root, "fleet");
    if (jfleet) {
        cJSON *count = cJSON_GetObjectItem(jfleet, "count");
        if (cJSON_IsNumber(count))
            es->game.fleet_count = (s32)count->valuedouble;
        cJSON *required = cJSON_GetObjectItem(jfleet, "required");
        if (cJSON_IsNumber(required))
            es->game.required_ships = (s32)required->valuedouble;
    }

    // allow_place
    cJSON *jallow = cJSON_GetObjectItem(root, "allow_place");
    if (jallow) {
        cJSON *sink = cJSON_GetObjectItem(jallow, "sink");
        if (cJSON_IsBool(sink)) es->allow_sink = cJSON_IsTrue(sink);
        cJSON *repel = cJSON_GetObjectItem(jallow, "repel");
        if (cJSON_IsBool(repel)) es->allow_repel = cJSON_IsTrue(repel);
        cJSON *max_j = cJSON_GetObjectItem(jallow, "max");
        if (cJSON_IsNumber(max_j)) es->allow_max = (s32)max_j->valuedouble;
    }

    // ui
    cJSON *jui = cJSON_GetObjectItem(root, "ui");
    if (jui) {
        cJSON *show_field = cJSON_GetObjectItem(jui, "show_field_default");
        if (cJSON_IsBool(show_field))
            es->show_field_default = cJSON_IsTrue(show_field);
    }

    cJSON_Delete(root);

    // Store file path
    snprintf(es->file_path, sizeof(es->file_path), "%s", path);

    // Generate planet textures
    planet_textures_generate(renderer, &es->game);

    es->selected       = SEL_NONE;
    es->textures_dirty = false;

    SDL_Log("editor_load: loaded '%s' (%d planets)", path, es->game.planet_count);
    return true;
}

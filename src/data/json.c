#include "data/json.h"
#include "data/fs.h"
#include <SDL3/SDL.h>
#include <cJSON.h>
#include <stdlib.h>

static bool parse_vec2(const cJSON *arr, Vec2 *v) {
    if (!cJSON_IsArray(arr) || cJSON_GetArraySize(arr) < 2) return false;
    v->x = (f32)cJSON_GetArrayItem(arr, 0)->valuedouble;
    v->y = (f32)cJSON_GetArrayItem(arr, 1)->valuedouble;
    return true;
}

bool json_load(const char *path, Game *game) {
    char *buf = NULL;
    long size = 0;
    if (!fs_read_file(path, &buf, &size)) {
        SDL_Log("json_load: failed to read file '%s'", path);
        return false;
    }

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        SDL_Log("json_load: parse error in '%s': %s", path, cJSON_GetErrorPtr());
        free(buf);
        return false;
    }

    // ppm
    cJSON *ppm = cJSON_GetObjectItem(root, "ppm");
    if (cJSON_IsNumber(ppm))
        game->cam.ppm = (f32)ppm->valuedouble;

    // start
    cJSON *start = cJSON_GetObjectItem(root, "start");
    if (start) {
        cJSON *pos = cJSON_GetObjectItem(start, "pos");
        if (pos) parse_vec2(pos, &game->ship.pos);

        cJSON *vel_max = cJSON_GetObjectItem(start, "vel_max");
        if (cJSON_IsNumber(vel_max))
            game->vel_max = (f32)vel_max->valuedouble;
    }

    // ship
    cJSON *ship = cJSON_GetObjectItem(root, "ship");
    if (ship) {
        cJSON *radius = cJSON_GetObjectItem(ship, "radius");
        if (cJSON_IsNumber(radius))
            game->ship.radius = (f32)radius->valuedouble;
    }

    // goal
    cJSON *goal = cJSON_GetObjectItem(root, "goal");
    if (goal) {
        cJSON *pos = cJSON_GetObjectItem(goal, "pos");
        if (pos) parse_vec2(pos, &game->goal.pos);

        cJSON *radius = cJSON_GetObjectItem(goal, "radius");
        if (cJSON_IsNumber(radius))
            game->goal.radius = (f32)radius->valuedouble;
    }

    // planets
    cJSON *planets = cJSON_GetObjectItem(root, "planets");
    if (cJSON_IsArray(planets)) {
        int count = cJSON_GetArraySize(planets);
        if (count > MAX_PLANETS) count = MAX_PLANETS;
        game->planet_count = count;

        for (int i = 0; i < count; i++) {
            cJSON *p = cJSON_GetArrayItem(planets, i);
            Planet *planet = &game->planets[i];

            cJSON *pos = cJSON_GetObjectItem(p, "pos");
            if (pos) parse_vec2(pos, &planet->pos);

            cJSON *radius = cJSON_GetObjectItem(p, "radius");
            if (cJSON_IsNumber(radius))
                planet->radius = (f32)radius->valuedouble;

            cJSON *mu = cJSON_GetObjectItem(p, "mu");
            if (cJSON_IsNumber(mu))
                planet->mu = (f32)mu->valuedouble;

            cJSON *eps = cJSON_GetObjectItem(p, "eps");
            if (cJSON_IsNumber(eps))
                planet->eps = (f32)eps->valuedouble;
        }
    }

    cJSON_Delete(root);
    free(buf);
    return true;
}

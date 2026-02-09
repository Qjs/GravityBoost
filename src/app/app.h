#pragma once
#include <stdbool.h>

typedef struct App App;

bool app_init(App *app);
void app_shutdown(App *app);

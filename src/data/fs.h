#pragma once
#include <stdbool.h>

bool fs_read_file(const char *path, char **out, long *size);

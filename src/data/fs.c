#include "data/fs.h"
#include <stdio.h>
#include <stdlib.h>

bool fs_read_file(const char *path, char **out, long *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return false;
    }

    long len = ftell(f);
    if (len < 0) {
        fclose(f);
        return false;
    }
    rewind(f);

    char *buf = malloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        return false;
    }

    size_t read = fread(buf, 1, (size_t)len, f);
    fclose(f);

    if ((long)read != len) {
        free(buf);
        return false;
    }

    buf[len] = '\0';
    *out = buf;
    if (size) *size = len;
    return true;
}

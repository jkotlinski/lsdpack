#include "rom.h"

#include <cstdio>
#include <cstdlib>

unsigned char sav[131072];

void load_sav(const char* path) {
    FILE* f = fopen(path, "rb");
    if (fread(sav, sizeof(sav), 1, f) != 1) {
        fprintf(stderr, "Reading .sav file '%s' failed!", path);
        exit(1);
    }
    fclose(f);
}


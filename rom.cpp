#include "rom.h"

#include <cstdio>
#include <cstdlib>

unsigned char rom[1048576];

void load_rom(const char* path) {
    FILE* f = fopen(path, "rb");
    if (fread(rom, sizeof(rom), 1, f) != 1) {
        fprintf(stderr, "Reading .gb file '%s' failed!", path);
        exit(1);
    }
    fclose(f);
}


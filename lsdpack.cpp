#include <cstdio>

#include "rom.h"
#include "sav.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: lsdpack <lsdj.gb> <lsdj.sav>");
        return 1;
    }
    load_rom(argv[1]);
    load_sav(argv[2]);
}

#include <cstdio>

#include "libgambatte/include/gambatte.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: lsdpack <lsdj.gb>");
        return 1;
    }
    gambatte::GB gameboy;
    gameboy.load(argv[1]);
}

#include "writer.h"

#include <cstdio>
#include <cstdlib>

FILE* f;
int bank;
int written;

#define STOP 0xff
#define LCD 0

static void new_bank() {
    ++bank;
    if (bank == 0x200) {
        fputs("ROM full!", stderr);
        exit(1);
    }
    fprintf(f, "SECTION \"MUSIC_%i\",ROMX,BANK[%i]\n", bank, bank);
    written = 0;
}

static void write_byte(unsigned char byte) {
    if (written == 0x4000) {
        new_bank();
    }
    fprintf(f, "DB %i\n", byte);
    ++written;
}

void record_song_start() {
    if (f == 0) {
        f = fopen("music.s", "w");
        new_bank();
    }
}

void record_song_stop() {
    write_byte(STOP);
}

void record_write(char addr, char data) {
    write_byte(addr);
    write_byte(data);
}

void record_lcd() {
    write_byte(LCD);
}

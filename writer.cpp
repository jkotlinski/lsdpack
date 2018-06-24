#include "writer.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

FILE* f;
int bank;
int written;

#define STOP 0xff
#define LCD 0

std::vector<int> song_bank;
std::vector<int> song_ptr;

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
    fprintf(f, "DB %i\n", byte);
    if (++written == 0x4000) {
        new_bank();
    }
}

void record_song_start() {
    if (f == 0) {
        f = fopen("music.s", "w");
        new_bank();
    }
    song_bank.push_back(bank);
    song_ptr.push_back(written + 0x4000);
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

void write_song_positions() {
    fputs("SECTION \"SONG_POSITIONS\",ROM0\n", f);
    fputs("SongBank\n", f);
    for (size_t i = 0; i < song_bank.size(); ++i) {
        fprintf(f, "dw %i\n", song_bank[i]);
    }
    fputs("SongPtr\n", f);
    for (size_t i = 0; i < song_ptr.size(); ++i) {
        fprintf(f, "dw %i\n", song_ptr[i]);
    }
    fputs("dw 0", f);
}

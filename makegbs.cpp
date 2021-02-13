/* lsdpack - standalone LSDj (Little Sound Dj) recorder + player {{{
   Copyright (C) 2018  Johan Kotlinski
   https://www.littlesounddj.com

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. }}} */

#include <cstdio>
#include <cstring>
#include <string>
#include "getopt.h"

#define PLAYER_ADDRESS 0x3d00

void verify(FILE* f, const char* path) {
    if (f != 0) {
        return;
    }

    char errormsg[1024];
    snprintf(errormsg, sizeof(errormsg),
             "Opening '%s' for write failed",
             path);
    perror(errormsg);
    exit(1);
}

std::string title = "<Title>";
std::string artist = "<Artist>";
std::string copyright = "<Copyright>";

void fputs_padded(const char* s, FILE* f) {
    if (strlen(s) > 32) {
        fprintf(stderr, "'%s' too long, max 32 characters\n", s);
        exit(1);
    }
    fputs(s, f);
    for (size_t i = strlen(s); i < 32; ++i) {
        fputc(0, f);
    }
}

void write_gbs_header(FILE* f) {
    fputs("GBS", f);
    fputc(1, f); // version 1

    fputc(1, f); // number of songs
    fputc(1, f); // first song

    // load address
    fputc(PLAYER_ADDRESS & 0xff, f);
    fputc(PLAYER_ADDRESS >> 8, f);

    // init address
    fputc((PLAYER_ADDRESS + 1) & 0xff, f);
    fputc((PLAYER_ADDRESS + 1) >> 8, f);

    // play address
    fputc((PLAYER_ADDRESS + 4) & 0xff, f);
    fputc((PLAYER_ADDRESS + 4) >> 8, f);

    // SP init
    fputc(0xfe, f);
    fputc(0xff, f);

    fputc(0x4a, f); // TMA
    fputc(6, f); // TAC

    printf("Title: %s\n", title.c_str());
    fputs_padded(title.c_str(), f);
    printf("Artist: %s\n", artist.c_str());
    fputs_padded(artist.c_str(), f);
    printf("Copyright: %s\n", copyright.c_str());
    fputs_padded(copyright.c_str(), f);
}

void print_help_and_exit() {
    fprintf(stderr, "usage: makegbs [-t <title>] [-a <artist>] [-c <copyright>] player.gb\n");
    exit(1);
}

int main(int argc, char* argv[]) {
    int c;
    while ((c = getopt(argc, argv, "t:c:a:")) != -1) {
        switch (c) {
            case 't':
                title = optarg;
                break;
            case 'a':
                artist = optarg;
                break;
            case 'c':
                copyright = optarg;
                break;
            default:
                print_help_and_exit();
        }
    }

    if (optind != argc - 1) {
        print_help_and_exit();
    }

    std::string gbs_path = argv[optind];
    gbs_path += 's';
    printf("Writing %s...\n", gbs_path.c_str());

    FILE* gbs_f = fopen(gbs_path.c_str(), "wb");
    verify(gbs_f, gbs_path.c_str());

    write_gbs_header(gbs_f);

    FILE* gb_f = fopen(argv[optind], "rb");
    verify(gb_f, argv[optind]);
    fseek(gb_f, PLAYER_ADDRESS, SEEK_SET);

    while (true) {
        c = fgetc(gb_f);
        if (c == EOF) {
            break;
        }
        fputc(c, gbs_f);
    }

    fclose(gbs_f);
    fclose(gb_f);

    puts("OK");
}

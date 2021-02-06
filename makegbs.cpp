#include <cstdio>
#include <cstring>
#include <string>
#include "getopt.h"

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

int song_count = 1;
std::string title = "<Title>";
std::string artist = "<Artist>";
std::string copyright = "<Copyright>";

void fputs_padded(const char* s, FILE* f) {
    if (strlen(s) > 32) {
        fprintf(stderr, "'%s' too long, max 32 characters", s);
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

    printf("Number of Songs: %i\n", song_count);
    fputc(song_count, f); // number of songs
    fputc(1, f); // first song

    // load address
    fputc(0, f);
    fputc(0x3f, f);

    // init address
    fputc(1, f);
    fputc(0x3f, f);

    // play address
    fputc(4, f);
    fputc(0x3f, f);

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

int main(int argc, char* argv[]) {
    int c;
    while ((c = getopt(argc, argv, "s:t:c:a:")) != -1) {
        switch (c) {
            case 's':
                song_count = atoi(optarg);
                break;
            case 't':
                title = optarg;
                break;
            case 'a':
                artist = optarg;
                break;
            case 'c':
                copyright = optarg;
                break;
        }
    }

    if (optind != argc - 1) {
        fprintf(stderr, "usage: makegbs [-s <number of songs>] [-a <artist>] [-t <title>] [-c <copyright>] player.gb");
        return 1;
    }

    std::string gbs_path = argv[optind];
    gbs_path += 's';
    printf("Writing %s...\n", gbs_path.c_str());

    FILE* gbs_f = fopen(gbs_path.c_str(), "wb");
    verify(gbs_f, gbs_path.c_str());

    write_gbs_header(gbs_f);

    FILE* gb_f = fopen(argv[optind], "rb");
    verify(gb_f, argv[optind]);
    fseek(gb_f, 0x3f00, SEEK_SET);

    while (true) {
        int c = fgetc(gb_f);
        if (c == EOF) {
            break;
        }
        fputc(c, gbs_f);
    }

    fclose(gbs_f);
    fclose(gb_f);

    puts("OK");
}

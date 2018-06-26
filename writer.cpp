#include "writer.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <vector>

FILE* f;

struct Location {
    int bank;
    int ptr;
};
static Location write_location;

#define SAMPLE 0xfe
#define STOP 0xff
#define LCD 0

#define START 0x100

static std::vector<Location> song_locations;
static std::vector<int> music_stream;

static void new_bank() {
    if (++write_location.bank == 0x200) {
        fputs("ROM full!", stderr);
        exit(1);
    }
    fprintf(f, "SECTION \"MUSIC_%i\",ROMX,BANK[%i]\n",
            write_location.bank, write_location.bank);
    write_location.ptr = 0x4000;
}

static void write_byte(unsigned char byte) {
    fprintf(f, "DB $%x\n", byte);
    if (++write_location.ptr == 0x8000) {
        new_bank();
    }
}

static std::deque<unsigned char> sample_buffer;

static bool sample_buffer_full() {
    return sample_buffer.size() == 44;
}

static bool sample_buffer_has_sample() {
    /* wav write:
     * $25=?
     * $1a=0
     * $30-3F=?
     * $1a=$80
     * $1e=?
     * $1d=?
     * $25=?  */
    return sample_buffer_full() &&
        sample_buffer[0] == 0x25 &&
        sample_buffer[2] == 0x1a &&
        sample_buffer[3] == 0 &&
        sample_buffer[4] == 0x30 &&
        sample_buffer[6] == 0x31 &&
        sample_buffer[8] == 0x32 &&
        sample_buffer[10] == 0x33 &&
        sample_buffer[12] == 0x34 &&
        sample_buffer[14] == 0x35 &&
        sample_buffer[16] == 0x36 &&
        sample_buffer[18] == 0x37 &&
        sample_buffer[20] == 0x38 &&
        sample_buffer[22] == 0x39 &&
        sample_buffer[24] == 0x3a &&
        sample_buffer[26] == 0x3b &&
        sample_buffer[28] == 0x3c &&
        sample_buffer[30] == 0x3d &&
        sample_buffer[32] == 0x3e &&
        sample_buffer[34] == 0x3f &&
        sample_buffer[36] == 0x1a &&
        sample_buffer[37] == 0x80 &&
        sample_buffer[38] == 0x1e &&
        sample_buffer[40] == 0x1d &&
        sample_buffer[42] == 0x25;
}

typedef std::map<std::vector<unsigned char>, Location> SampleLocations;
static SampleLocations sample_locations;

static void write_sample_buffer() {
    std::vector<unsigned char> sample_contents;
    for (int i = 5; i < 5 + 32; i += 2) {
        sample_contents.push_back(sample_buffer[i]);
    }
    Location sample_location;
    if (sample_locations.find(sample_contents) == sample_locations.end()) {
        sample_locations[sample_contents] = write_location;
        sample_location = write_location;
        for (size_t i = 0; i < sample_contents.size(); ++i) {
            write_byte(sample_contents[i]);
        }
    } else {
        sample_location = sample_locations[sample_contents];
    }
    music_stream.push_back(SAMPLE);
    assert(sample_location.bank < 0x100);
    music_stream.push_back(sample_location.bank);
    music_stream.push_back(sample_location.ptr & 0xff);
    music_stream.push_back(sample_location.ptr >> 8);
    music_stream.push_back(sample_buffer[39]); // freq
    music_stream.push_back(sample_buffer[41]); // freq
    sample_buffer.clear();
}

static void flush_sample_buffer() {
    music_stream.insert(music_stream.end(), sample_buffer.begin(), sample_buffer.end());
    sample_buffer.clear();
}

static void record_byte(unsigned char byte) {
    if (sample_buffer_full()) {
        music_stream.push_back(sample_buffer.front());
        sample_buffer.pop_front();
    }
    sample_buffer.push_back(byte);

    if (sample_buffer_has_sample()) {
        write_sample_buffer();
    }
}

// -----

void record_song_start() {
    if (f == 0) {
        f = fopen("music.s", "w");
        new_bank();
    }
    music_stream.push_back(START);
}

void record_song_stop() {
    music_stream.push_back(STOP);
}

void record_write(char addr, char data) {
    record_byte(addr);
    record_byte(data);
}

void record_lcd() {
    record_byte(LCD);
}

static void write_song_locations() {
    fputs("SECTION \"SONG_POSITIONS\",ROM0\n", f);
    fputs("EXPORT SongBank,SongPtr\n", f);
    fputs("SongBank\n", f);
    for (size_t i = 0; i < song_locations.size(); ++i) {
        fprintf(f, "dw %i\n", song_locations[i].bank);
    }
    fputs("SongPtr\n", f);
    for (size_t i = 0; i < song_locations.size(); ++i) {
        fprintf(f, "dw $%x\n", song_locations[i].ptr);
    }
    fputs("dw 0", f);
}

void record_complete() {
    flush_sample_buffer();
    size_t i = 0;
    while (i < music_stream.size()) {
        if (music_stream[i] == START) {
            song_locations.push_back(write_location);
        } else {
            write_byte(music_stream[i]);
        }
        ++i;
    }
    write_song_locations();
}

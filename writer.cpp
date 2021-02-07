#include "writer.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <vector>

// #define RECORD_WRITES

#ifdef RECORD_WRITES
FILE* music_file = fopen("music", "wb");
FILE* sample_file = fopen("samples", "wb");
#endif

static FILE* f;

struct Location {
    int bank;
    int ptr;
};
static Location write_location;

static bool gbs_mode;

#define LYC 0
#define SAMPLE 1
#define STOP 2
#define NEXT_BANK 3
#define VOLUME_DOWN_PU0 4
#define VOLUME_DOWN_PU1 5
#define VOLUME_DOWN_NOI 6

#define LYC_END_MASK 0x80

#define START               0x100
#define TYPE_ADDR           0x200
#define TYPE_LYC            0x400
#define TYPE_SAMPLE         0x800

static std::vector<Location> song_locations;
static std::vector<unsigned int> music_stream;

static int sample_count;

static void reset() {
    sample_count = 0;
    song_locations.clear();
    music_stream.clear();
    write_location.bank = 0;
    write_location.ptr = 0;
    fclose(f);
    f = 0;
}

static void new_bank() {
    // In GBS mode, assume max 32 banks for compatibility.
    const int max_bank_count = (gbs_mode ? 0x20 : 0x200);
    if (++write_location.bank == max_bank_count) {
        fputs("ROM full!", stderr);
        exit(1);
    }
    fprintf(f, "SECTION \"MUSIC_%i\",ROMX,BANK[%i]\n",
            write_location.bank, write_location.bank);
    write_location.ptr = 0x4000;
}

static void write_byte(unsigned int byte) {
    if (write_location.ptr > 0x7ff8) {
        if ((byte & TYPE_ADDR) || (byte & TYPE_LYC) || (byte & TYPE_SAMPLE) || (write_location.ptr == 0x7fff)) {
            fprintf(f, "DB 3 ; next bank\n");
            new_bank();
        }
    }
    fprintf(f, "DB $%x\n", byte & 0xff);
    ++write_location.ptr;
    assert(write_location.ptr < 0x8000);
}

static std::deque<unsigned int> sample_buffer;

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
        sample_buffer[0] == (0x25 | TYPE_ADDR) &&
        sample_buffer[2] == (0x1a | TYPE_ADDR) &&
        sample_buffer[3] == 0 &&
        sample_buffer[4] == (0x30 | TYPE_ADDR) &&
        sample_buffer[6] == (0x31 | TYPE_ADDR) &&
        sample_buffer[8] == (0x32 | TYPE_ADDR) &&
        sample_buffer[10] == (0x33 | TYPE_ADDR) &&
        sample_buffer[12] == (0x34 | TYPE_ADDR) &&
        sample_buffer[14] == (0x35 | TYPE_ADDR) &&
        sample_buffer[16] == (0x36 | TYPE_ADDR) &&
        sample_buffer[18] == (0x37 | TYPE_ADDR) &&
        sample_buffer[20] == (0x38 | TYPE_ADDR) &&
        sample_buffer[22] == (0x39 | TYPE_ADDR) &&
        sample_buffer[24] == (0x3a | TYPE_ADDR) &&
        sample_buffer[26] == (0x3b | TYPE_ADDR) &&
        sample_buffer[28] == (0x3c | TYPE_ADDR) &&
        sample_buffer[30] == (0x3d | TYPE_ADDR) &&
        sample_buffer[32] == (0x3e | TYPE_ADDR) &&
        sample_buffer[34] == (0x3f | TYPE_ADDR) &&
        sample_buffer[36] == (0x1a | TYPE_ADDR) &&
        sample_buffer[37] == 0x80 &&
        sample_buffer[38] == (0x1e | TYPE_ADDR) &&
        sample_buffer[40] == (0x1d | TYPE_ADDR) &&
        sample_buffer[42] == (0x25 | TYPE_ADDR);
}

static void optimize_volume_decreases() {
    if (sample_buffer.size() < 15 * 2) {
        return;
    }
    const size_t tail_start = sample_buffer.size() - 15 * 2;

    // Check values.
    for (size_t i = tail_start + 1; i < tail_start + 15 * 2; i += 2) {
        if (sample_buffer[i] != 8) {
            return;
        }
    }

    // Check addresses.
    unsigned int register_addr = 0;

    for (size_t i = tail_start; i < tail_start + 15 * 2; i += 2) {
        unsigned int addr = sample_buffer[i];
        if (!(addr & TYPE_ADDR)) {
            return;
        }
        switch (addr & 0x7f) {
            case 0x12:
            case 0x17:
            case 0x21:
                if (register_addr && ((addr & 0x7f) != register_addr)) {
                    return;
                }
                register_addr = addr & 0x7f;
                break;
            default:
                return;
        }
    }

    // OK, there is a volume decrease at sample_buffer tail.
    // Let's rewrite it in the optimized form.
    std::deque<unsigned int> new_sample_buffer;
    // Preserve head.
    for (size_t i = 0; i < tail_start; ++i) {
        new_sample_buffer.push_back(sample_buffer[i]);
    }

    unsigned int byte;
    switch (register_addr) {
        case 0x12:
            byte = VOLUME_DOWN_PU0;
            break;
        case 0x17:
            byte = VOLUME_DOWN_PU1;
            break;
        case 0x21:
            byte = VOLUME_DOWN_NOI;
            break;
        default:
            assert(false);
    }
    new_sample_buffer.push_back(byte);

    sample_buffer = new_sample_buffer;
}

typedef std::map<std::vector<unsigned char>, Location> SampleLocations;
static SampleLocations sample_locations;

static void write_sample_buffer() {
    std::vector<unsigned char> sample_contents;
    for (int i = 5; i < 5 + 32; i += 2) {
        sample_contents.push_back(sample_buffer[i]);
    }
    SampleLocations::iterator sample_location = sample_locations.find(sample_contents);
    if (sample_location == sample_locations.end()) {
        sample_location = sample_locations.insert(std::make_pair(sample_contents, write_location)).first;
        for (size_t i = 0; i < sample_contents.size(); ++i) {
            write_byte(sample_contents[i]);
            ++sample_count;
#ifdef RECORD_WRITES
            fwrite(&sample_contents[i], 1, 1, sample_file);
#endif
        }
    }
    music_stream.push_back(SAMPLE | TYPE_SAMPLE);
    assert(sample_location->second.bank < 0x100);
    music_stream.push_back(sample_location->second.bank);
    music_stream.push_back(sample_location->second.ptr & 0xff);
    music_stream.push_back(sample_location->second.ptr >> 8);
    music_stream.push_back(sample_buffer[39]); // freq
    music_stream.push_back(sample_buffer[41]); // freq
    sample_buffer.clear();
}

static void flush_sample_buffer() {
    for (size_t i = 0; i < sample_buffer.size(); ++i) {
        music_stream.push_back(sample_buffer[i]);
    }
    sample_buffer.clear();
}

static void record_byte(unsigned int byte) {
    if (sample_buffer_full()) {
        music_stream.push_back(sample_buffer.front());
        sample_buffer.pop_front();
    }
    sample_buffer.push_back(byte);

    if (sample_buffer_has_sample()) {
        write_sample_buffer();
    } else {
        optimize_volume_decreases();
    }
}

// -----

void record_song_start(const char* out_path) {
    if (f == 0) {
        f = fopen(out_path, "w");
        if (f == 0) {
            char errormsg[1024];
            snprintf(errormsg, sizeof(errormsg),
                     "Opening '%s' for write failed",
                     out_path);
            perror(errormsg);
            exit(1);
        }
        new_bank();
    }
    music_stream.push_back(START);
}

static void log_written_bytes() {
    static int last_music_size;
    printf("wrote %i sample bytes, %i music bytes\n",
            sample_count,
            (int)music_stream.size() - last_music_size);
    sample_count = 0;
    last_music_size = music_stream.size();
}

void record_song_stop() {
    flush_sample_buffer();
    music_stream.push_back(STOP);
    log_written_bytes();
}

void record_write(unsigned char addr, unsigned char data) {
    record_byte(addr | TYPE_ADDR);
    record_byte(data);
}

void record_lcd() {
    if (sample_buffer.size() >= 2 &&
            (sample_buffer[sample_buffer.size() - 2] & TYPE_ADDR) &&
            !(sample_buffer[sample_buffer.size() - 2] & LYC_END_MASK)) {
        // Records LYC by setting LYC_END_MASK bit on last written address.
        sample_buffer[sample_buffer.size() - 2] |= LYC_END_MASK;
    } else {
        record_byte(LYC | TYPE_LYC);
    }
}

static void write_song_locations() {
    fputs(gbs_mode
            ?  "SECTION \"SONG_LOCATIONS\",ROM0[$3fe0]\n"
            :  "SECTION \"SONG_LOCATIONS\",ROM0\n",
            f);
    fputs("SongLocations::\n", f);
    for (size_t i = 0; i < song_locations.size(); ++i) {
        fprintf(f,
                "dw %i, $%x\n",
                song_locations[i].bank,
                song_locations[i].ptr);
    }
    fputs("SongLocationsEnd::\n", f);
}

void write_music_to_disk() {
    size_t i = 0;
    while (i < music_stream.size()) {
        if (music_stream[i] == START) {
            song_locations.push_back(write_location);
        } else {
            write_byte(music_stream[i]);
#ifdef RECORD_WRITES
            fwrite(&music_stream[i], 1, 1, music_file);
#endif
        }
        ++i;
    }
    write_song_locations();
    reset();
}

void enable_gbs_mode() {
    gbs_mode = true;
}

#include "writer.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

static int regs[0x100];

#define LYC             0
#define SAMPLE          1
#define STOP            2
#define NEXT_BANK       3
#define AMP_DOWN_PU0    4
#define AMP_DOWN_PU1    5
#define AMP_DOWN_NOI    6
#define PITCH_PU0       7
#define PITCH_PU1       8
#define PITCH_WAV       9
#define WAIT            10

#define LYC_END_MASK 0x80

#define START               0x100
#define CMD_FLAG            0x200

static std::vector<Location> song_locations;
static std::vector<unsigned int> music_stream;

static int sample_count;

static void reset() {
    sample_count = 0;
    song_locations.clear();
    music_stream.clear();
    memset(regs, -1, sizeof(regs));
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

static void fprint_cmd_comment(FILE* f, unsigned int cmd) {
    if (!(cmd >> 8)) {
        return;
    }
    fprintf(f, "\t; ");
    if (cmd & 0x80) {
        fprintf(f, "LYC+");
    }
    switch (cmd & 0x7f) {
        case LYC:
            fprintf(f, "LYC");
            break;
        case SAMPLE:
            fprintf(f, "SAMPLE");
            break;
        case STOP:
            fprintf(f, "STOP");
            break;
        case NEXT_BANK:
            fprintf(f, "NEXT_BANK");
            break;
        case AMP_DOWN_PU0:
            fprintf(f, "AMP_DOWN_PU0");
            break;
        case AMP_DOWN_PU1:
            fprintf(f, "AMP_DOWN_PU1");
            break;
        case AMP_DOWN_NOI:
            fprintf(f, "AMP_DOWN_NOI");
            break;
        case PITCH_PU0:
            fprintf(f, "PITCH_PU0");
            break;
        case PITCH_PU1:
            fprintf(f, "PITCH_PU1");
            break;
        case PITCH_WAV:
            fprintf(f, "PITCH_WAV");
            break;
        case WAIT:
            fprintf(f, "WAIT");
            break;
        case 0x10:
            fprintf(f, "pu0 sweep");
            break;
        case 0x11:
            fprintf(f, "pu0 length/wave");
            break;
        case 0x12:
            fprintf(f, "pu0 env");
            break;
        case 0x13:
            fprintf(f, "pu0 pitch lsb");
            break;
        case 0x14:
            fprintf(f, "pu0 pitch msb");
            break;

        case 0x16:
            fprintf(f, "pu1 length/wave");
            break;
        case 0x17:
            fprintf(f, "pu1 env");
            break;
        case 0x18:
            fprintf(f, "pu1 pitch lsb");
            break;
        case 0x19:
            fprintf(f, "pu1 pitch msb");
            break;

        case 0x1a:
            fprintf(f, "wav on/off");
            break;
        case 0x1b:
            fprintf(f, "wav length");
            break;
        case 0x1c:
            fprintf(f, "wav env");
            break;
        case 0x1d:
            fprintf(f, "wav pitch lsb");
            break;
        case 0x1e:
            fprintf(f, "wav pitch msb");
            break;

        case 0x20:
            fprintf(f, "noi length");
            break;
        case 0x21:
            fprintf(f, "noi env");
            break;
        case 0x22:
            fprintf(f, "noi wave");
            break;
        case 0x23:
            fprintf(f, "noi trig");
            break;

        default:
            fprintf(f, "%x", cmd & 0x7f);
    }
}

static void write_byte(unsigned int byte) {
    if (write_location.ptr > 0x7ff8) {
        if ((byte & CMD_FLAG) || (write_location.ptr == 0x7fff)) {
            fprintf(f, "DB 3 ; next bank\n");
            new_bank();
        }
    }
    fprintf(f, "DB $%x", byte & 0xff);
    fprint_cmd_comment(f, byte);
    fprintf(f, "\n");
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
        sample_buffer[0] == (0x25 | CMD_FLAG) &&
        sample_buffer[2] == (0x1a | CMD_FLAG) &&
        sample_buffer[3] == 0 &&
        sample_buffer[4] == (0x30 | CMD_FLAG) &&
        sample_buffer[6] == (0x31 | CMD_FLAG) &&
        sample_buffer[8] == (0x32 | CMD_FLAG) &&
        sample_buffer[10] == (0x33 | CMD_FLAG) &&
        sample_buffer[12] == (0x34 | CMD_FLAG) &&
        sample_buffer[14] == (0x35 | CMD_FLAG) &&
        sample_buffer[16] == (0x36 | CMD_FLAG) &&
        sample_buffer[18] == (0x37 | CMD_FLAG) &&
        sample_buffer[20] == (0x38 | CMD_FLAG) &&
        sample_buffer[22] == (0x39 | CMD_FLAG) &&
        sample_buffer[24] == (0x3a | CMD_FLAG) &&
        sample_buffer[26] == (0x3b | CMD_FLAG) &&
        sample_buffer[28] == (0x3c | CMD_FLAG) &&
        sample_buffer[30] == (0x3d | CMD_FLAG) &&
        sample_buffer[32] == (0x3e | CMD_FLAG) &&
        sample_buffer[34] == (0x3f | CMD_FLAG) &&
        sample_buffer[36] == (0x1a | CMD_FLAG) &&
        sample_buffer[37] == 0x80 &&
        sample_buffer[38] == (0x1e | CMD_FLAG) &&
        sample_buffer[40] == (0x1d | CMD_FLAG) &&
        sample_buffer[42] == (0x25 | CMD_FLAG);
}

static void optimize_pitch() {
    if (sample_buffer.size() < 4) {
        return;
    }
    const size_t tail_start = sample_buffer.size() - 4;
    int cmd = 0;
    int new_lsb;
    int new_msb;
    if (sample_buffer[tail_start] == (0x13 | CMD_FLAG) &&
            sample_buffer[tail_start + 2] == (0x14 | CMD_FLAG)) {
        new_lsb = sample_buffer[tail_start + 1];
        new_msb = sample_buffer[tail_start + 3];
        bool trig = new_msb & 0x80;
        if (new_msb == regs[0x14] && !trig) {
            // msb is redundant
            cmd = (new_lsb == regs[0x13])
                ? 0 // lsb is redundant, too
                : (0x13 | CMD_FLAG); // only set lsb
        } else {
            cmd = PITCH_PU0 | CMD_FLAG;
        }
        regs[0x13] = new_lsb;
        regs[0x14] = new_msb;
    } else if (sample_buffer[tail_start] == (0x18 | CMD_FLAG) &&
            sample_buffer[tail_start + 2] == (0x19 | CMD_FLAG)) {
        new_lsb = sample_buffer[tail_start + 1];
        new_msb = sample_buffer[tail_start + 3];
        bool trig = new_msb & 0x80;
        if (new_msb == regs[0x19] && !trig) {
            // msb is redundant
            cmd = (new_lsb == regs[0x18])
                ? 0 // lsb is redundant, too
                : (0x18 | CMD_FLAG); // only set lsb
        } else {
            cmd = PITCH_PU1 | CMD_FLAG;
        }
        regs[0x18] = new_lsb;
        regs[0x19] = new_msb;
    } else if (sample_buffer[tail_start] == (0x1d | CMD_FLAG) &&
            sample_buffer[tail_start + 2] == (0x1e | CMD_FLAG)) {
        new_lsb = sample_buffer[tail_start + 1];
        new_msb = sample_buffer[tail_start + 3];
        bool trig = new_msb & 0x80;
        if (new_msb == regs[0x1e] && !trig) {
            // msb is redundant
            cmd = (new_lsb == regs[0x1d])
                ? 0 // lsb is redundant, too
                : (0x1d | CMD_FLAG); // only set lsb
        } else {
            cmd = PITCH_WAV | CMD_FLAG;
        }
        regs[0x1d] = new_lsb;
        regs[0x1e] = new_msb;
    } else {
        return;
    }
    sample_buffer.resize(tail_start);
    if (!cmd) {
        return;
    }
    sample_buffer.push_back(cmd);
    sample_buffer.push_back(new_lsb);
    if (!(cmd & 0x10)) {
        sample_buffer.push_back(new_msb);
    }
}

static void optimize_wait() {
    if (sample_buffer.size() < 3) {
        return;
    }
    const size_t tail_start = sample_buffer.size() - 3;
    if (sample_buffer[tail_start] == (LYC | CMD_FLAG) &&
            sample_buffer[tail_start + 1] == (LYC | CMD_FLAG) &&
            sample_buffer[tail_start + 2] == (LYC | CMD_FLAG)) {
        // LYC:LYC:LYC => WAIT:0
        sample_buffer.resize(tail_start);
        sample_buffer.push_back(WAIT | CMD_FLAG);
        sample_buffer.push_back(2);
    } else if (sample_buffer[tail_start] == (WAIT | CMD_FLAG) &&
            sample_buffer[tail_start + 1] != 0xff &&
            sample_buffer[tail_start + 2] == (LYC | CMD_FLAG)) {
        // WAIT:duration:LYC => WAIT:(duration+1)
        sample_buffer.resize(sample_buffer.size() - 1);
        ++sample_buffer[tail_start + 1];
    }
}

/* LSDj 8.8.0+ soft envelope problem:
 * To decrease volume on CGB, the byte 8 is written 15 times to
 * either of addresses 0xff12, 0xff17 or 0xff21.
 * To improve sound on DMG and reduce ROM/CPU usage, replace this
 * with commands AMP_DOWN_XXX
 */
static void optimize_envelope() {
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
        if (!(addr & CMD_FLAG)) {
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
    sample_buffer.resize(tail_start);

    unsigned int byte;
    switch (register_addr) {
        case 0x12:
            byte = AMP_DOWN_PU0 | CMD_FLAG;
            break;
        case 0x17:
            byte = AMP_DOWN_PU1 | CMD_FLAG;
            break;
        case 0x21:
            byte = AMP_DOWN_NOI | CMD_FLAG;
            break;
        default:
            assert(false);
    }
    sample_buffer.push_back(byte);
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
    music_stream.push_back(SAMPLE | CMD_FLAG);
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
        optimize_envelope();
        optimize_pitch();
        optimize_wait();
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
    record_byte(addr | CMD_FLAG);
    record_byte(data);
}

void record_lcd() {
    for (int i = (int)sample_buffer.size() - 1; i >= 0; --i) {
        if (!(sample_buffer[i] & CMD_FLAG)) {
            continue;
        }
        if (sample_buffer[i] == (LYC | CMD_FLAG)) {
            break;
        }
        if (sample_buffer[i] == (WAIT | CMD_FLAG)) {
            break;
        }
        if (sample_buffer[i] & LYC_END_MASK) {
            break;
        }
        // Records LYC by setting LYC_END_MASK bit on last written address.
        sample_buffer[i] |= LYC_END_MASK;
        return;
    }
    record_byte(LYC | CMD_FLAG);
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

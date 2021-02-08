#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <vector>

#include "rule.h"

#include "rule_redundant_write.h"
#include "rule_wait.h"
#include "rule_envelope.h"
#include "rule_pitch.h"
#include "rule_lyc.h"

// #define RECORD_WRITES

#ifdef RECORD_WRITES
FILE* music_file = fopen("music", "wb");
FILE* sample_file = fopen("samples", "wb");
#endif

class Writer {
    public:
        explicit Writer(bool gbs_mode)
            : gbs_mode(gbs_mode)
        {
            memset(regs, -1, sizeof(regs));
            f = 0;
            gbs_mode = false;
            sample_count = 0;
            curr_sample_bank = -1;
            curr_sample_bank = -1;
            last_music_size = 0;
        }

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
            memset(regs, -1, sizeof(regs));
            music_stream.push_back(SONG_START);
        }

        void log_written_bytes() {
            printf("wrote %i sample bytes, %i music bytes\n",
                    sample_count,
                    (int)music_stream.size() - last_music_size);
            sample_count = 0;
            last_music_size = music_stream.size();
        }

        void record_song_stop() {
            music_stream.push_back(STOP | CMD_FLAG);
        }

        void record_write(unsigned char addr, unsigned char data) {
            record_byte(addr | CMD_FLAG);
            record_byte(data);
        }

        void record_lcd() {
            record_byte(LYC | CMD_FLAG);
        }

        void write_song_locations() {
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

        void optimize_rule(Rule& rule) {
            std::deque<unsigned int> window;
            std::vector<unsigned int> new_music_stream;

            for (size_t i = 0; i < music_stream.size(); ++i) {
                window.push_back(music_stream[i]);
                if (window.size() > rule.width()) {
                    new_music_stream.push_back(window.front());
                    window.pop_front();
                }
                if (window.size() == rule.width()) {
                    rule.transform(window);
                }
            }
            while (window.size()) {
                new_music_stream.push_back(window.front());
                window.pop_front();
            }
            music_stream = new_music_stream;
        }

        void optimize_music_stream() {
            RedundantWriteRule pan(0x25);
            RedundantWriteRule pu0_sweep(0x10);
            RedundantWriteRule pu0_length(0x11);
            RedundantWriteRule pu1_length(0x16);
            RedundantWriteRule wav_length(0x1b);
            RedundantWriteRule wav_volume(0x1c);
            RedundantWriteRule noi_length(0x20);
            RedundantWriteRule noi_wave(0x22);

            WaitRule wait;
            EnvelopeRule envelope;
            PitchRule pitch;
            LycRule lyc;

            optimize_rule(pan);
            optimize_rule(pu0_sweep);
            optimize_rule(pu0_length);
            optimize_rule(pu1_length);
            optimize_rule(wav_length);
            optimize_rule(wav_volume);
            optimize_rule(noi_length);
            optimize_rule(noi_wave);

            optimize_rule(wait);
            optimize_rule(envelope);
            optimize_rule(pitch);
            optimize_rule(lyc);

            /*
            if (sample_buffer_full()) {
                sample_buffer.pop_front();
            }
            sample_buffer.push_back(byte);

            if (sample_buffer_has_sample()) {
                write_sample_buffer();
            } else {
            }
            */
        }

        void write_music_to_disk() {
            optimize_music_stream();

            size_t song_start = 0;
            size_t i = 0;
            while (i < music_stream.size()) {
                if (music_stream[i] == SONG_START) {
                    song_locations.push_back(write_location);
                    song_start = i;
                } else {
                    write_byte(music_stream[i]);
#ifdef RECORD_WRITES
                    fwrite(&music_stream[i], 1, 1, music_file);
#endif
                    if (music_stream[i] == (STOP | CMD_FLAG)) {
                        printf("Wrote %i bytes\n", (int)(i - song_start));
                    }
                }
                ++i;
            }
            write_song_locations();
        }

    private:
        FILE* f;

        const bool gbs_mode;

        struct Location {
            Location() : bank(0), ptr(0) {}
            int bank;
            int ptr;
        };
        Location write_location;

        unsigned int regs[0x100];

        typedef std::map<std::vector<unsigned char>, Location> SampleLocations;
        SampleLocations sample_locations;

        std::deque<unsigned int> sample_buffer;

        std::vector<Location> song_locations;
        std::vector<unsigned int> music_stream;

        int sample_count;
        int curr_sample_bank;
        int curr_sample_address;

        int last_music_size;

        // -----

        void new_bank() {
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

        void fprint_cmd_comment(unsigned int cmd) {
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
                case SAMPLE_NEXT:
                    fprintf(f, "SAMPLE_NEXT");
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

                case 0x25:
                    fprintf(f, "pan");
                    break;

                default:
                    fprintf(f, "%x", cmd & 0x7f);
            }
        }

        void write_byte(unsigned int byte) {
            if (write_location.ptr > 0x7ff8) {
                if ((byte & CMD_FLAG) || (write_location.ptr == 0x7fff)) {
                    fprintf(f, "DB 3 ; next bank\n");
                    new_bank();
                }
            }
            fprintf(f, "DB $%x", byte & 0xff);
            fprint_cmd_comment(byte);
            fprintf(f, "\n");
            ++write_location.ptr;
            assert(write_location.ptr < 0x8000);
        }

        bool sample_buffer_full() {
            return sample_buffer.size() == 44;
        }

        bool sample_buffer_has_sample() {
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

        void write_sample_buffer() {
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
            unsigned int new_pitch_lsb = sample_buffer[39];
            unsigned int new_pitch_msb = sample_buffer[41];
            sample_buffer.clear();

            if (new_pitch_lsb == regs[0x1d] && new_pitch_msb == regs[0x1e] &&
                    curr_sample_bank == sample_location->second.bank &&
                    curr_sample_address == sample_location->second.ptr - 0x10) {
                sample_buffer.push_back(SAMPLE_NEXT | CMD_FLAG);
            } else if (curr_sample_bank == sample_location->second.bank &&
                    curr_sample_address == sample_location->second.ptr) {
                if (regs[0x1d] != new_pitch_lsb) {
                    sample_buffer.push_back(0x1d | CMD_FLAG);
                    sample_buffer.push_back(new_pitch_lsb);
                }
                sample_buffer.push_back(0x1e | CMD_FLAG);
                sample_buffer.push_back(new_pitch_msb);
            } else {
                sample_buffer.push_back(SAMPLE | CMD_FLAG);
                assert(sample_location->second.bank < 0x100);
                sample_buffer.push_back(sample_location->second.bank);
                sample_buffer.push_back(sample_location->second.ptr & 0xff);
                sample_buffer.push_back(sample_location->second.ptr >> 8);
                sample_buffer.push_back(new_pitch_lsb);
                sample_buffer.push_back(new_pitch_msb);
            }

            regs[0x1d] = new_pitch_lsb;
            regs[0x1e] = new_pitch_msb;
            curr_sample_bank = sample_location->second.bank;
            curr_sample_address = sample_location->second.ptr;
        }

        void flush_sample_buffer() {
            for (size_t i = 0; i < sample_buffer.size(); ++i) {
                music_stream.push_back(sample_buffer[i]);
            }
            sample_buffer.clear();
        }

        void record_byte(unsigned int byte) {
            music_stream.push_back(byte);
        }

};

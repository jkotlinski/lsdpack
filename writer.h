#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <map>
#include <vector>

#include "location.h"
#include "rule.h"
#include "rule_redundant_write.h"
#include "rule_wait.h"
#include "rule_envelope.h"
#include "rule_pitch.h"
#include "rule_lyc.h"
#include "rule_sample.h"
#include "rule_interrupted_sample.h"

class Writer {
    public:
        explicit Writer(bool gbs_mode)
            : gbs_mode(gbs_mode)
        {
            memset(regs, -1, sizeof(regs));
            f = 0;
            gbs_mode = false;
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
            InterruptedSampleRule interruptedSampleRule;

            optimize_rule(interruptedSampleRule);
            optimize_rule(sample_rule);

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
        }

        void write_music_to_disk() {
            optimize_music_stream();

            write_samples();

            size_t song_start = 0;
            size_t i = 0;
            int song_count = 0;
            while (i < music_stream.size()) {
                if (music_stream[i] == SONG_START) {
                    fprintf(f, "; === Start song %i\n", ++song_count);
                    song_locations.push_back(write_location);
                    song_start = i;
                } else {
                    write_byte(music_stream[i]);
                    if (music_stream[i] == (STOP | CMD_FLAG)) {
                        printf("Song %i: %i bytes\n", song_count, (int)(i - song_start));
                    }
                }
                ++i;
            }
            write_song_locations();
        }

    private:
        FILE* f;

        const bool gbs_mode;

        Location write_location;

        unsigned int regs[0x100];

        std::vector<Location> song_locations;
        std::vector<unsigned int> music_stream;

        int last_music_size;

        SampleRule sample_rule;

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

                case 0x24:
                    fprintf(f, "channel volume");
                    break;
                case 0x25:
                    fprintf(f, "pan");
                    break;
                case 0x26:
                    fprintf(f, "sound off/on");
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

        void record_byte(unsigned int byte) {
            music_stream.push_back(byte);
        }

        void write_samples() {
            std::vector<unsigned char> samples = sample_rule.get_samples();
            for (size_t i = 0; i < samples.size(); i += 0x10) {
                fprintf(f, "DB ");
                for (size_t j = i; j < i + 0x10; ++j) {
                    fprintf(f, "$%x", samples[j]);
                    if ((j & 0xf) != 0xf) {
                        fprintf(f, ",");
                    }
                }
                fputs("\n", f);
                write_location.ptr += 0x10;
                if (write_location.ptr == 0x8000) {
                    new_bank();
                }
            }
            printf("Wrote %i samples\n", (int)samples.size());
        }
};

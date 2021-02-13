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

#include "writer.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <vector>

#include "rules/rule.h"
#include "rules/rule_redundant_write.h"
#include "rules/rule_envelope.h"
#include "rules/rule_pitch.h"
#include "rules/rule_lyc.h"
#include "rules/rule_sample.h"
#include "rules/rule_interrupted_sample.h"
#include "rules/rule_repeat_command.h"

static bool optimizations_disabled;

Writer::Writer(bool gbs_mode) : gbs_mode(gbs_mode) {
    memset(regs, -1, sizeof(regs));
    f = 0;
    last_music_size = 0;
}

void Writer::record_song_start(const char* out_path) {
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
    music_stream.push_back(FLAG_SONG_START);
}

void Writer::record_song_stop() {
    music_stream.push_back(CMD_SONG_STOP | FLAG_CMD);
}

void Writer::record_write(unsigned char addr, unsigned char data) {
    record_byte(addr | FLAG_CMD);
    record_byte(data);
}

void Writer::record_lcd() {
    record_byte(FLAG_CMD | CMD_END_TICK);
}

void Writer::write_song_locations() {
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

void Writer::optimize_rule(Rule& rule) {
    std::deque<unsigned int> window;
    std::vector<unsigned int> new_music_stream;

    for (size_t i = 0; i < music_stream.size(); ++i) {
        window.push_back(music_stream[i]);
        if (window.size() > rule.window_size()) {
            new_music_stream.push_back(window.front());
            window.pop_front();
        }
        if (window.size() == rule.window_size()) {
            rule.transform(window);
        }
    }
    while (window.size()) {
        new_music_stream.push_back(window.front());
        window.pop_front();
    }
    music_stream = new_music_stream;
}

void Writer::optimize_music_stream() {
    if (optimizations_disabled) {
        return;
    }

    RedundantWriteRule pan(0x25);
    RedundantWriteRule pu0_sweep(0x10);
    RedundantWriteRule pu0_length(0x11);
    RedundantWriteRule pu1_length(0x16);
    RedundantWriteRule wav_length(0x1b);
    RedundantWriteRule pu0_pitch_lsb(0x13);
    RedundantWriteRule pu1_pitch_lsb(0x18);
    RedundantWriteRule wav_pitch_lsb(0x1d);
    RedundantWriteRule wav_volume(0x1c);
    RedundantWriteRule noi_length(0x20);
    RedundantWriteRule noi_wave(0x22);

    EnvelopeRule envelope;
    PitchRule pitch;
    LycRule lyc;
    InterruptedSampleRule interruptedSampleRule;
    RepeatCommandRule repeatCommandRule;

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

    optimize_rule(envelope);
    optimize_rule(pitch);
    optimize_rule(pu0_pitch_lsb);
    optimize_rule(pu1_pitch_lsb);
    optimize_rule(wav_pitch_lsb);
    optimize_rule(lyc);
    optimize_rule(repeatCommandRule);
}

// Inserts CMD_NEXT_BANK where needed.
void Writer::insert_new_bank_cmds() {
    int last_cmd = 0;
    int write_ptr = write_location.ptr;

    for (size_t i = 0; i < music_stream.size(); ++i) {
        if (music_stream[i] & FLAG_CMD) {
            last_cmd = i;
        }

        ++write_ptr;
        if (write_ptr == 0x7fff) {
            music_stream.insert(music_stream.begin() + last_cmd, FLAG_CMD | CMD_NEXT_BANK);
            write_ptr = 0x4000;
            i = last_cmd + 1;
        }
    }
}

void Writer::write_music_to_disk() {
    optimize_music_stream();

    write_samples();

    insert_new_bank_cmds();

    size_t song_start = 0;
    size_t i = 0;
    int song_count = 0;
    while (i < music_stream.size()) {
        if (music_stream[i] == FLAG_SONG_START) {
            fprintf(f, "; === Start song %i\n", ++song_count);
            song_locations.push_back(write_location);
            song_start = i;
        } else {
            write_byte(music_stream[i]);
            if (music_stream[i] == (FLAG_CMD | CMD_SONG_STOP)) {
                printf("Song %i: %i bytes\n", song_count, (int)(i - song_start));
            }
        }
        ++i;
    }
    write_song_locations();
}

// ----- private

void Writer::new_bank() {
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
    if (cmd & FLAG_REPEAT) {
        fprintf(f, "FLAG_REPEAT + ");
    }
    switch (cmd & 0x3f) {
        case CMD_END_TICK:
            fprintf(f, "CMD_END_TICK");
            break;
        case CMD_SAMPLE_START:
            fprintf(f, "CMD_SAMPLE_START");
            break;
        case CMD_SAMPLE_NEXT:
            fprintf(f, "CMD_SAMPLE_NEXT");
            break;
        case CMD_SONG_STOP:
            fprintf(f, "CMD_SONG_STOP");
            break;
        case CMD_NEXT_BANK:
            fprintf(f, "CMD_NEXT_BANK");
            break;
        case CMD_AMP_DEC_PU0:
            fprintf(f, "CMD_AMP_DEC_PU0");
            break;
        case CMD_AMP_DEC_PU1:
            fprintf(f, "CMD_AMP_DEC_PU1");
            break;
        case CMD_AMP_DEC_NOI:
            fprintf(f, "CMD_AMP_DEC_NOI");
            break;
        case CMD_PITCH_PU0:
            fprintf(f, "CMD_PITCH_PU0");
            break;
        case CMD_PITCH_PU1:
            fprintf(f, "CMD_PITCH_PU1");
            break;
        case CMD_PITCH_WAV:
            fprintf(f, "CMD_PITCH_WAV");
            break;

        case 0x10:
            fprintf(f, "[pu0 sweep]");
            break;
        case 0x11:
            fprintf(f, "[pu0 length/wave]");
            break;
        case 0x12:
            fprintf(f, "[pu0 env]");
            break;
        case 0x13:
            fprintf(f, "[pu0 pitch lsb]");
            break;
        case 0x14:
            fprintf(f, "[pu0 pitch msb]");
            break;

        case 0x16:
            fprintf(f, "[pu1 length/wave]");
            break;
        case 0x17:
            fprintf(f, "[pu1 env]");
            break;
        case 0x18:
            fprintf(f, "[pu1 pitch lsb]");
            break;
        case 0x19:
            fprintf(f, "[pu1 pitch msb]");
            break;

        case 0x1a:
            fprintf(f, "[wav on/off]");
            break;
        case 0x1b:
            fprintf(f, "[wav length]");
            break;
        case 0x1c:
            fprintf(f, "[wav env]");
            break;
        case 0x1d:
            fprintf(f, "[wav pitch lsb]");
            break;
        case 0x1e:
            fprintf(f, "[wav pitch msb]");
            break;

        case 0x20:
            fprintf(f, "[noi length]");
            break;
        case 0x21:
            fprintf(f, "[noi env]");
            break;
        case 0x22:
            fprintf(f, "[noi wave]");
            break;
        case 0x23:
            fprintf(f, "[noi trig]");
            break;

        case 0x24:
            fprintf(f, "[channel volume]");
            break;
        case 0x25:
            fprintf(f, "[pan]");
            break;
        case 0x26:
            fprintf(f, "[sound off/on]");
            break;

        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
        case 0x3a:
        case 0x3b:
        case 0x3c:
        case 0x3d:
        case 0x3e:
        case 0x3f:
            fprintf(f, "[wave]");
            break;

        default:
            fprintf(stderr, "Write to unknown register: %x\n", cmd & 0x7f);
            assert(false);
    }
    if (cmd & FLAG_END_TICK) {
        fprintf(f, " + FLAG_END_TICK");
    }
}

void Writer::write_byte(unsigned int byte) {
    assert(write_location.ptr >= 0x4000);
    assert(write_location.ptr < 0x8000);

    fprintf(f, "DB $%x", byte & 0xff);
    fprint_cmd_comment(f, byte);
    fprintf(f, "\n");

    if (byte == (FLAG_CMD | CMD_NEXT_BANK)) {
        new_bank();
    } else {
        ++write_location.ptr;
    }
}

void Writer::record_byte(unsigned int byte) {
    music_stream.push_back(byte);
}

void Writer::write_samples() {
    std::vector<unsigned char> samples = sample_rule.get_samples();
    for (size_t i = 0; i < samples.size(); i += 0x10) {
        assert(write_location.ptr >= 0x4000);
        assert(write_location.ptr < 0x8000);

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

void Writer::disable_optimizations() {
    optimizations_disabled = true;
}

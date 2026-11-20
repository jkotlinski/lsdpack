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
#include <cstdlib>
#include <cstring>
#include <fstream>

#include "gambatte.h"

#include "input.h"
#include "writer.h"
#include "dumpwriter.h"
#include "getopt.h"

gambatte::GB gameboy;
Input input;
std::string out_path;

IWriter* writer;

/* LSDj 9.2.0 moved its sound tick from the LCD (LYC) interrupt to the timer
 * interrupt. Ticks are recorded from whichever interrupt the loaded ROM uses. */
enum TickSource {
    TICK_AUTO_DETECT,
    TICK_LCD,   // LSDj up to 9.1.x
    TICK_TIMER, // LSDj 9.2.0+
};

TickSource tick_source_option = TICK_AUTO_DETECT;
TickSource tick_source = TICK_LCD;

void run_one_frame() {
    size_t samples = 35112;
    static gambatte::uint_least32_t audioBuffer[35112 + 2064];
    gameboy.runFor(0, 0, &audioBuffer[0], samples);
}

void wait(float seconds) {
    for (float i = 0.f; i < 60.f * seconds; ++i) {
        run_one_frame();
    }
}

void press(unsigned key, float seconds = 0.1f) {
    input.press(key);
    if (!gameboy.isCgb() && seconds > 0.1f) {
        seconds *= 2; // compensates for slow CPU
    }
    wait(seconds);
}

bool load_song(int position) {
    press(SELECT);
    press(SELECT | UP);
    press(0);
    for (int i = 0; i < 16; ++i) {
        press(DOWN); // go to bottom
        press(0);
    }
    press(A);
    press(0);
    press(A);
    press(0);
    for (int i = 0; i < 32; ++i) {
        press(UP); // go to top
        press(0);
    }
    for (int i = 0; i < position; ++i) {
        press(DOWN);
        press(0);
    }
    // press song name
    press(A);
    press(0);
    // discard changes
    press(LEFT);
    press(0);
    press(A);
    // wait until song is loaded
    press(0, 5);
    if (gameboy.isSongEmpty()) {
        return false;
    }
    return true;
}

bool sound_enabled;

void play_song() {
    int seconds_elapsed = 0;
    sound_enabled = false;
    writer->record_song_start(out_path.c_str());
    press(START);
    if (!sound_enabled) {
        fputs("Aborted: Song did not start.\n", stderr);
        exit(1);
    }
    while (sound_enabled) {
        wait(1);

        if (++seconds_elapsed == 60 * 60) {
            fputs("Aborted: Song still playing after one hour. Please add a HFF command to song end to stop recording.\n", stderr);
            exit(1);
        }
    }
}

void on_ff_write(char p, char data, unsigned long cycle) {
    if (p < 0x10 || p >= 0x40) {
        return; // not sound
    }
    switch (p) {
        case 0x26:
            if (sound_enabled && !data) {
                writer->record_song_stop();
                sound_enabled = false;
                return;
            }
            sound_enabled = data;
            break;
    }
    if (sound_enabled) {
        writer->record_write(p, data, cycle);
    }
}

void on_lcd_interrupt() {
    if (sound_enabled && tick_source == TICK_LCD) {
        writer->record_lcd();
    }
}

void on_timer_interrupt() {
    if (sound_enabled && tick_source == TICK_TIMER) {
        writer->record_lcd();
    }
}

/* Reads the version from the ROM title, e.g. "LSDj-v9.2.L" or "LSDj-v4.9.4". */
TickSource detect_tick_source(const char* path) {
    char title[16] = {0};
    std::ifstream rom(path, std::ios::binary);
    rom.seekg(0x134);
    rom.read(title, 15);
    const char* v = strstr(title, "v");
    int major = 0;
    int minor = 0;
    if (!v || sscanf(v + 1, "%d.%d", &major, &minor) != 2) {
        fprintf(stderr, "Could not detect LSDj version of %s, assuming < 9.2.0. "
                "Use -t or -l to choose the tick source.\n", path);
        return TICK_LCD;
    }
    printf("Detected LSDj v%d.%d\n", major, minor);
    return (major > 9 || (major == 9 && minor >= 2)) ? TICK_TIMER : TICK_LCD;
}

void make_out_path(const char* in_path, std::string suffix) {
    if (strlen(in_path) < strlen("a.gb")) {
        return;
    }

    out_path = in_path;
    const size_t dot = out_path.rfind('.');
    if (dot != std::string::npos) {
        out_path.erase(dot); // strip .gb/.gbc
    }
    out_path += suffix;
    out_path.replace(out_path.begin(), out_path.begin() + 1 + out_path.rfind('/'), "");
    out_path.replace(out_path.begin(), out_path.begin() + 1 + out_path.rfind('\\'), "");
    printf("Recording to '%s'\n", out_path.c_str());
}

void load_gb(const char* path, bool dmg_mode) {
    tick_source = tick_source_option == TICK_AUTO_DETECT
        ? detect_tick_source(path)
        : tick_source_option;
    if (tick_source == TICK_TIMER) {
        /* In CGB double speed mode the timer runs at twice the rate, so the
         * recorded song would play at half tempo in player.s. */
        puts("Recording timer interrupts (LSDj 9.2.0+), using emulated DMG");
        dmg_mode = true;
    } else {
        puts("Recording LCD interrupts (LSDj < 9.2.0)");
    }
    unsigned flags = dmg_mode ? gameboy.FORCE_DMG : 0;
    if (gameboy.load(path, flags)) {
        fprintf(stderr, "Loading %s failed\n", path);
        exit(1);
    }
    printf("Loaded %s\n", path);
    press(0, 3);

    /* Press B to skip the first LittleFM screen, if the ROM is patched with it */
    press(B);
    press(0);
}

void record_gb(int argc, char* argv[], bool dmg_mode) {
    make_out_path(argv[optind], ".s");
    writer = new Writer(false);
    unsigned flags = dmg_mode ? gameboy.FORCE_DMG : 0;

    for (; optind < argc; ++optind) {
        load_gb(argv[optind], flags);

        for (int song_index = 0; song_index < 32; ++song_index) {
            if (load_song(song_index)) {
                printf("Playing song %i...\n", song_index + 1);
                play_song();
            }
        }
    }
    writer->write_music_to_disk();

    delete writer;
}

void record_gbs(int argc, char* argv[], bool dmg_mode) {
    unsigned flags = dmg_mode ? gameboy.FORCE_DMG : 0;
    for (; optind < argc; ++optind) {
        load_gb(argv[optind], flags);

        for (int song_index = 0; song_index < 32; ++song_index) {
            writer = new Writer(true);

            if (load_song(song_index)) {
                printf("Playing song %i...\n", song_index + 1);

                char suffix[20];
                snprintf(suffix, sizeof(suffix), "-%i.s", song_index + 1);
                make_out_path(argv[optind], suffix);

                play_song();
                writer->write_music_to_disk();
            }

            delete writer;
        }
    }
}

void record_dump(int argc, char* argv[], bool dmg_mode) {
    writer = new DumpWriter();
    unsigned flags = dmg_mode ? gameboy.FORCE_DMG : 0;

    for (; optind < argc; ++optind) {
        load_gb(argv[optind], flags);

        for (int song_index = 0; song_index < 32; ++song_index) {
            if (load_song(song_index)) {
                printf("Playing song %i...\n", song_index + 1);

                char suffix[20];
                snprintf(suffix, sizeof(suffix), "-%i.txt", song_index + 1);
                make_out_path(argv[optind], suffix);

                play_song();
            }
        }
    }
    delete writer;
}

void print_help_and_exit() {
    puts("usage: lsdpack [-g] [-r] [-d] [-t|-l] [lsdj.gb lsdj2.gb ...]");
    puts("");
    puts("-g: .gbs conversion");
    puts("-r: raw register dump; optimizations disabled");
    puts("-d: record using emulated DMG (default: CGB; always DMG for LSDj 9.2.0+)");
    puts("-t: record timer interrupts, as LSDj 9.2.0+ (default: detect from ROM title)");
    puts("-l: record LCD interrupts, as LSDj < 9.2.0 (default: detect from ROM title)");
    exit(1);
}

int main(int argc, char* argv[]) {
    bool gbs_mode = false;
    bool dump_mode = false;
    bool dmg_mode = false;
    unsigned flags = 0;

    int c;
    while ((c = getopt(argc, argv, "grdtl")) != -1) {
        switch (c) {
            case 'g':
                puts(".gbs mode enabled");
                gbs_mode = true;
                break;
            case 'r':
                puts("register dump mode enabled");
                dump_mode = true;
                break;
            case 'd':
                puts("recording using emulated DMG");
                dmg_mode = true;
                break;
            case 't':
                tick_source_option = TICK_TIMER;
                break;
            case 'l':
                tick_source_option = TICK_LCD;
                break;
            default:
                print_help_and_exit();
        }
    }

    if (argc <= optind) {
        print_help_and_exit();
    }

    gameboy.setInputGetter(&input);
    gameboy.setWriteHandler(on_ff_write);
    gameboy.setLcdHandler(on_lcd_interrupt);
    gameboy.setTimerHandler(on_timer_interrupt);

    if (dmg_mode) {
        flags |= gameboy.FORCE_DMG;
    }
    if (gbs_mode) {
        record_gbs(argc, argv, flags);
    } else if (dump_mode) {
        record_dump(argc, argv, flags);
    } else {
        record_gb(argc, argv, flags);
    }

    puts("OK");
}

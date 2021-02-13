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

#include "gambatte.h"

#include "input.h"
#include "writer.h"
#include "getopt.h"

gambatte::GB gameboy;
Input input;
std::string out_path;

Writer* writer;

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
    wait(seconds);
}

bool load_song(int position) {
    press(SELECT);
    press(SELECT | UP);
    press(0);
    press(DOWN, 3);
    press(0);
    press(A);
    press(0);
    press(A);
    press(0);
    press(UP, 5); // scroll to top
    press(0);
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
    input.press(START);
    writer->record_song_start(out_path.c_str());
    do {
        wait(1);

        if (++seconds_elapsed == 60 * 60) {
            fputs("Aborted: Song still playing after one hour. Please add a HFF command to song end to stop recording.\n", stderr);
            exit(1);
        }
    } while(sound_enabled);
}

void on_ff_write(char p, char data) {
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
        writer->record_write(p, data);
    }
}

void on_lcd_interrupt() {
    if (sound_enabled) {
        writer->record_lcd();
    }
}

void make_out_path(const char* in_path, std::string suffix) {
    if (strlen(in_path) < strlen("a.gb")) {
        return;
    }

    out_path = in_path;
    // .gb => .s
    out_path.replace(out_path.end() - 2, out_path.end(), "s");
    out_path.insert(out_path.length() - 2, suffix);
    out_path.replace(out_path.begin(), out_path.begin() + 1 + out_path.rfind('/'), "");
    out_path.replace(out_path.begin(), out_path.begin() + 1 + out_path.rfind('\\'), "");
    printf("Recording to '%s'\n", out_path.c_str());
}

void load_gb(const char* path) {
    if (gameboy.load(path)) {
        fprintf(stderr, "Loading %s failed\n", path);
        exit(1);
    }
    printf("Loaded %s\n", path);
    press(0, 3);
}

void record_gb(int argc, char* argv[]) {
    make_out_path(argv[optind], "");
    writer = new Writer(false);

    for (; optind < argc; ++optind) {
        load_gb(argv[optind]);

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

void record_gbs(int argc, char* argv[]) {
    for (; optind < argc; ++optind) {
        load_gb(argv[optind]);

        for (int song_index = 0; song_index < 32; ++song_index) {
            writer = new Writer(true);

            if (load_song(song_index)) {
                printf("Playing song %i...\n", song_index + 1);

                char suffix[20];
                snprintf(suffix, sizeof(suffix), "-%i", song_index + 1);
                make_out_path(argv[optind], suffix);

                play_song();
                writer->write_music_to_disk();
            }

            delete writer;
        }
    }
}

void print_help_and_exit() {
    puts("usage: lsdpack [-g] [-r] [lsdj.gb lsdj2.gb ...]");
    puts("");
    puts("-g: .gbs conversion");
    puts("-r: raw register writes; optimizations disabled");
    exit(1);
}

int main(int argc, char* argv[]) {
    bool gbs_mode = false;

    int c;
    while ((c = getopt(argc, argv, "gr")) != -1) {
        switch (c) {
            case 'g':
                puts(".gbs mode enabled");
                gbs_mode = true;
                break;
            case 'r':
                puts("disabled optimizations");
                Writer::disable_optimizations();
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

    if (gbs_mode) {
        record_gbs(argc, argv);
    } else {
        record_gb(argc, argv);
    }

    puts("OK");
}

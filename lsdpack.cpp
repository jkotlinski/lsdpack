#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "gambatte.h"

#include "input.h"
#include "writer.h"
#include "getopt.h"

int written_songs;
gambatte::GB gameboy;
Input input;
std::string out_path;

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
    printf("Song %i...\n", ++written_songs);
    return true;
}

bool sound_enabled;

void play_song() {
    int seconds_elapsed = 0;
    sound_enabled = false;
    input.press(START);
    record_song_start(out_path.c_str());
    do {
        wait(1);

        if (++seconds_elapsed == 60 * 60) {
            fputs("Aborted: Song still playing after one hour. Please add a HFF command to song end to stop recording.", stderr);
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
                record_song_stop();
                sound_enabled = false;
                return;
            }
            sound_enabled = data;
            break;
    }
    if (sound_enabled) {
        record_write(p, data);
    }
}

void on_lcd_interrupt() {
    if (sound_enabled) {
        record_lcd();
    }
}

void make_out_path(const char* in_path) {
    out_path = in_path;
    // .gb => .s
    out_path.replace(out_path.end() - 2, out_path.end(), "s");
    out_path.replace(out_path.begin(), out_path.begin() + 1 + out_path.rfind('/'), "");
    out_path.replace(out_path.begin(), out_path.begin() + 1 + out_path.rfind('\\'), "");
    printf("Recording to '%s'\n", out_path.c_str());
}

int main(int argc, char* argv[]) {
    int c;
    while ((c = getopt(argc, argv, "g")) != -1) {
        switch (c) {
            case 'g':
                puts(".gbs mode enabled");
                enable_gbs_mode();
                break;
        }
    }

    if (argc <= optind) {
        fprintf(stderr, "usage: lsdpack [-g] [lsdj.gb lsdj2.gb ...]");
        return 1;
    }

    make_out_path(argv[optind]);

    gameboy.setInputGetter(&input);
    gameboy.setWriteHandler(on_ff_write);
    gameboy.setLcdHandler(on_lcd_interrupt);

    for (; optind < argc; ++optind) {
        printf("Loading %s...\n", argv[optind]);
        gameboy.load(argv[optind]);
        press(0, 3);

        int song_index = 0;
        while (load_song(song_index++)) {
            play_song();
        }
    }

    write_music_to_disk();

    puts("OK");
}

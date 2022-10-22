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

#include "iwriter.h"

class DumpWriter : public IWriter {
    public:
        void record_song_start(const char* out_path) override {
            last_cycle = 0;

            if (f == nullptr) {
                f = fopen(out_path, "w");
                if (f == nullptr) {
                    char errormsg[1024];
                    snprintf(errormsg, sizeof(errormsg),
                            "Opening '%s' for write failed",
                            out_path);
                    perror(errormsg);
                    exit(1);
                }
            }
        }

        void record_write(unsigned char addr, unsigned char data, unsigned long cycle) override {
            if (last_cycle == 0) {
                last_cycle = cycle;
            }
            fprintf(f, "%08lx ff%02x=%02x\n", cycle - last_cycle, addr, data);
            last_cycle = cycle;
        }

        void record_song_stop() override {
            fclose(f);
            f = nullptr;
        }

        void record_lcd() override {}
        void write_music_to_disk() override {}

    private:
        FILE* f = nullptr;
        unsigned long last_cycle = 0;
};

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

#include <vector>

#include "iwriter.h"
#include "rules/sample_rule.h"

class Rule;

class Writer : public IWriter {
    public:
        explicit Writer(bool gbs_mode);

        void record_song_start(const char* out_path);
        void record_song_stop();
        void record_write(unsigned char addr, unsigned char data, unsigned long cycle);
        void record_lcd();
        void write_music_to_disk();

    private:
        void write_song_locations();
        void optimize_rule(Rule& rule);
        void optimize_music_stream();

        FILE* f;

        const bool gbs_mode;

        struct Location {
            Location() : bank(0), ptr(0) {}
            int bank;
            int ptr;
        };
        Location write_location;

        unsigned int regs[0x100];

        std::vector<Location> song_locations;
        std::vector<unsigned int> music_stream;

        int last_music_size;

        SampleRule sample_rule;

        // -----

        void new_bank();
        void write_byte(unsigned int byte);
        void record_byte(unsigned int byte);
        void write_samples();
        void insert_new_bank_cmds();
};

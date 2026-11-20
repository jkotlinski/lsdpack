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

#include "rule.h"

#include <cstring>

class RedundantWriteRule : public Rule {
    public:
        /* Drops a write to `reg` when it carries the same value as the previous
         * write to that register.
         *
         * `require_zero_mask` guards value bits that must be zero for the drop
         * to be safe. It is used for the pulse length/duty registers (NR11/NR16)
         * where bits 7-6 are duty (a plain no-op when unchanged) but bits 5-0
         * reload the hardware length counter: passing 0x3f there means "only
         * eliminate pure duty writes, never one that also reloads length".
         *
         * `reset_on_song_start` clears the shadow at each FLAG_SONG_START. Songs
         * play standalone (NR52 is cleared on song start, wiping every APU
         * register), so a shadow carried across the boundary would wrongly drop
         * the new song's establishing write. Off by default to keep the
         * long-standing rules bit-identical; set for rules added later.
         */
        explicit RedundantWriteRule(unsigned int reg,
                                    unsigned int require_zero_mask = 0,
                                    bool reset_on_song_start = false)
            : reg(reg)
            , require_zero_mask(require_zero_mask)
            , reset_on_song_start(reset_on_song_start)
            , reg_state(-1) { }

        size_t window_size() const override { return 2; }

        void transform(std::deque<unsigned int>& bytes) override {
            if (bytes[0] == FLAG_SONG_START) {
                if (reset_on_song_start) {
                    reg_state = -1;
                }
                return;
            }
            if (bytes[0] != (reg | FLAG_CMD)) {
                return;
            }

            if (bytes[1] == reg_state && !(bytes[1] & require_zero_mask)) {
                bytes.clear();
            } else {
                reg_state = bytes[1];
            }
        }

    private:
        unsigned int reg;
        unsigned int require_zero_mask;
        bool reset_on_song_start;
        unsigned int reg_state;
};

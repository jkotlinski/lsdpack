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

class LycRule : public Rule {
    public:
        size_t window_size() const override { return 8; }

        // CMD:*:LYC => CMD|0x80:*
        void transform(std::deque<unsigned int>& bytes) override {
            if (bytes[7] != (FLAG_CMD | CMD_END_TICK)) {
                return;
            }

            int i;
            for (i = 6; i >= 0; --i) {
                if (bytes[i] & FLAG_CMD) {
                    break;
                }
            }
            if (i == -1) {
                return;
            }

            if (bytes[i] == (FLAG_CMD | CMD_END_TICK)) {
                return;
            }
            if (bytes[i] & FLAG_END_TICK) {
                return;
            }

            bytes[i] |= FLAG_END_TICK;
            bytes.resize(7);
        }
};

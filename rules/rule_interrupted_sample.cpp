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

#include "rule_interrupted_sample.h"

#include <cassert>

void InterruptedSampleRule::transform(std::deque<unsigned int>& bytes) {
    const bool interrupted_sample =
        (bytes[0] == (0x25 | FLAG_CMD) && bytes[4] == (0x30 | FLAG_CMD) && bytes[43] == (0x25 | FLAG_CMD)) ||
        (bytes[0] == (0x25 | FLAG_CMD) && bytes[5] == (0x30 | FLAG_CMD) && bytes[43] == (0x25 | FLAG_CMD));

    if (!interrupted_sample) {
        return;
    }

    size_t i = 0;
    while (i < bytes.size()) {
        if (bytes[i] == (CMD_END_TICK | FLAG_CMD)) {
            break;
        }
        ++i;
    }
    assert(i != bytes.size());
    while (i < bytes.size() - 1) {
        bytes[i] = bytes[i + 1];
        ++i;
    }
    bytes[i] = (CMD_END_TICK | FLAG_CMD);
}

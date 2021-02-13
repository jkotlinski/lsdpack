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

#include "rule_envelope.h"

#include <cassert>

void EnvelopeRule::transform(std::deque<unsigned int>& bytes) {
    // Check values.
    for (size_t i = 1; i < 15 * 2; i += 2) {
        if (bytes[i] != 8) {
            return;
        }
    }

    // Check addresses.
    unsigned int register_addr = 0;

    for (size_t i = 0; i < 15 * 2; i += 2) {
        unsigned int addr = bytes[i];
        if (!(addr & FLAG_CMD)) {
            return;
        }
        switch (addr & 0x7f) {
            case 0x12:
            case 0x17:
            case 0x21:
                if (register_addr && ((addr & 0x7f) != register_addr)) {
                    return;
                }
                register_addr = addr & 0x7f;
                break;
            default:
                return;
        }
    }

    // OK, there is a volume decrease.
    // Let's rewrite it in the optimized form.
    bytes.clear();

    unsigned int byte;
    switch (register_addr) {
        case 0x12:
            byte = CMD_AMP_DEC_PU0 | FLAG_CMD;
            break;
        case 0x17:
            byte = CMD_AMP_DEC_PU1 | FLAG_CMD;
            break;
        case 0x21:
            byte = CMD_AMP_DEC_NOI | FLAG_CMD;
            break;
        default:
            assert(false);
    }
    bytes.push_back(byte);
}

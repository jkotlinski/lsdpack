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

#include "rule_pitch.h"

PitchRule::PitchRule() :
    pu0_lsb_state(-1),
    pu0_msb_state(-1),
    pu1_lsb_state(-1),
    pu1_msb_state(-1),
    wav_lsb_state(-1),
    wav_msb_state(-1) {}

void PitchRule::transform(std::deque<unsigned int>& bytes) {
    int cmd = 0;
    unsigned int new_lsb = bytes[1];
    unsigned int new_msb = bytes[3];
    bool trig = new_msb & 0x80;
    if (bytes[0] == (0x13 | FLAG_CMD) && bytes[2] == (0x14 | FLAG_CMD)) {
        if (new_msb == pu0_msb_state && !trig) {
            // msb is redundant
            cmd = (new_lsb == pu0_lsb_state)
                ? 0 // lsb is redundant, too
                : (0x13 | FLAG_CMD); // only set lsb
        } else {
            cmd = CMD_PITCH_PU0 | FLAG_CMD;
        }
        pu0_msb_state = new_msb;
        pu0_lsb_state = new_lsb;
    } else if (bytes[0] == (0x18 | FLAG_CMD) && bytes[2] == (0x19 | FLAG_CMD)) {
        if (new_msb == pu1_msb_state && !trig) {
            // msb is redundant
            cmd = (new_lsb == pu1_lsb_state)
                ? 0 // lsb is redundant, too
                : (0x18 | FLAG_CMD); // only set lsb
        } else {
            cmd = CMD_PITCH_PU1 | FLAG_CMD;
        }
        pu1_msb_state = new_lsb;
        pu1_lsb_state = new_msb;
    } else if (bytes[0] == (0x1d | FLAG_CMD) && bytes[2] == (0x1e | FLAG_CMD)) {
        if (new_msb == wav_msb_state && !trig) {
            // msb is redundant
            cmd = (new_lsb == wav_lsb_state)
                ? 0 // lsb is redundant, too
                : (0x1d | FLAG_CMD); // only set lsb
        } else {
            cmd = CMD_PITCH_WAV | FLAG_CMD;
        }
        wav_msb_state = new_msb;
        wav_lsb_state = new_lsb;
    } else {
        return;
    }

    bytes.clear();

    if (!cmd) {
        return;
    }

    bytes.push_back(cmd);
    bytes.push_back(new_lsb);
    if (!(cmd & 0x10)) {
        bytes.push_back(new_msb);
    }
}

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

#include "pitch_rule.h"

PitchRule::PitchRule() :
    pu0_lsb_state(-1),
    pu0_msb_state(-1),
    pu1_lsb_state(-1),
    pu1_msb_state(-1) {}

void PitchRule::transform(std::deque<unsigned int>& bytes) {
    // CH3 synth pitch: fold NR33/NR34 ($1d,$1e) into one CMD_PITCH_WAV
    // (3 bytes instead of 4). Fold only, no redundant-write elimination:
    // $1d/$1e are shared with sample playback (CMD_SAMPLE_* writes them behind
    // this rule's back) and LSDj emits them in both orders, so a reliable
    // shadow is not available. The player's .pitch_wav writes $1d then $1e -
    // byte-identical APU effect to the two raw writes it replaces. Wave-heavy
    // tracks emit this every tick, so 4->3 is a big win (e.g. -67 KB / song).
    if (bytes[0] == (0x1d | FLAG_CMD) && bytes[2] == (0x1e | FLAG_CMD)) {
        const unsigned int lsb = bytes[1];
        const unsigned int msb = bytes[3];
        bytes.clear();
        bytes.push_back(CMD_PITCH_WAV | FLAG_CMD);
        bytes.push_back(lsb);
        bytes.push_back(msb);
        return;
    }

    int cmd = 0;
    unsigned int new_lsb = bytes[1];
    unsigned int new_msb = bytes[3];
    bool trig = new_msb & 0x80;
    if (bytes[0] == (0x13 | FLAG_CMD) && bytes[2] == (0x14 | FLAG_CMD)) {
        // TODO: Redundant write detection that takes sweep into account.
        cmd = CMD_PITCH_PU0 | FLAG_CMD;
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
        pu1_lsb_state = new_lsb;
        pu1_msb_state = new_msb;
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

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

#pragma once

#include <cstdio>
#include <deque>

enum Cmds {
    CMD_END_TICK,
    CMD_SAMPLE_START,
    CMD_SONG_STOP,
    CMD_NEXT_BANK,
    CMD_AMP_DEC_PU0,
    CMD_AMP_DEC_PU1,
    CMD_AMP_DEC_NOI,
    CMD_PITCH_PU0,
    CMD_PITCH_PU1,
    CMD_PITCH_WAV,
    CMD_SAMPLE_NEXT
};

enum Flags {
    FLAG_END_TICK   = 0x80,
    FLAG_REPEAT     = 0x40,
    FLAG_SONG_START = 0x100,
    FLAG_CMD        = 0x200
};

class Rule {
    public:
        virtual size_t window_size() const = 0;
        virtual void transform(std::deque<unsigned int>& bytes) = 0;
};

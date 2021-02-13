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

#include "inputgetter.h"

enum Button { A     = 0x01, B    = 0x02, SELECT = 0x04, START = 0x08,
              RIGHT = 0x10, LEFT = 0x20, UP     = 0x40, DOWN  = 0x80 };

struct Input : public gambatte::InputGetter {
    Input() : pressed(0) {}

	virtual unsigned operator()() {
        return pressed;
    }

    void press(unsigned button) {
        pressed = button;
    }

    unsigned pressed;
};

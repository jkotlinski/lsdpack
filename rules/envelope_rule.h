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

class EnvelopeRule : public Rule {
    public:
        size_t window_size() const override { return 15 * 2; }

        /* LSDj 8.8.0+ soft envelope problem:
         * To decrease volume on CGB, the byte 8 is written 15 times to
         * either of addresses 0xff12, 0xff17 or 0xff21.
         * To improve sound on DMG and reduce ROM/CPU usage, replace this
         * with commands AMP_DOWN_XXX
         */
        void transform(std::deque<unsigned int>& bytes) override;
};

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
        explicit RedundantWriteRule(unsigned int reg) : reg(reg), reg_state(-1) { }

        size_t window_size() const override { return 2; }

        void transform(std::deque<unsigned int>& bytes) override {
            if (bytes[0] != (reg | FLAG_CMD)) {
                return;
            }

            if (bytes[1] == reg_state) {
                bytes.clear();
            } else {
                reg_state = bytes[1];
            }
        }

    private:
        unsigned int reg;
        unsigned int reg_state;
};

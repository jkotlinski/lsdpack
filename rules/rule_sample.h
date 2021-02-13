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

#include <map>
#include <vector>

#include "rule.h"

class SampleRule : public Rule {
    private:
        struct Location {
            Location() : bank(0), ptr(0) {}
            int bank;
            int ptr;
        };

        typedef std::map<std::vector<unsigned char>, Location> SampleLocations;
        SampleLocations sample_locations;

        int sample_count;
        int curr_sample_bank;
        int curr_sample_address;

        unsigned int pitch_lsb_state;
        unsigned int pitch_msb_state;

        Location write_location;

        std::vector<unsigned char> all_samples;

    public:
        SampleRule();

        std::vector<unsigned char> get_samples() { return all_samples; }

        size_t window_size() const override { return 44; }

        void transform(std::deque<unsigned int>& bytes) override;
};

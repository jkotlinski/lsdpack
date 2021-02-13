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

#include "rule_sample.h"

#include <cassert>

SampleRule::SampleRule() :
    sample_count(0),
    curr_sample_bank(-1),
    curr_sample_address(-1),
    pitch_lsb_state(-1),
    pitch_msb_state(-1) {
        write_location.bank = 1;
        write_location.ptr = 0x4000;
    }

void SampleRule::transform(std::deque<unsigned int>& bytes) {
    const bool wav_write =
        bytes[0] == (0x25 | FLAG_CMD) &&
        bytes[4] == (0x30 | FLAG_CMD) &&
        bytes[42] == (0x25 | FLAG_CMD);

    if (!wav_write) {
        return;
    }

    std::vector<unsigned char> sample_contents;
    for (int i = 5; i < 5 + 32; i += 2) {
        sample_contents.push_back(bytes[i]);
    }
    SampleLocations::iterator sample_location = sample_locations.find(sample_contents);
    if (sample_location == sample_locations.end()) {
        sample_location = sample_locations.insert(std::make_pair(sample_contents, write_location)).first;
        write_location.ptr += 0x10;
        if (write_location.ptr == 0x8000) {
            write_location.ptr = 0x4000;
            ++write_location.bank;
        }
        for (size_t i = 0; i < sample_contents.size(); ++i) {
            all_samples.push_back(sample_contents[i]);
        }
    }

    unsigned int new_pitch_lsb = bytes[39];
    unsigned int new_pitch_msb = bytes[41];
    bytes.clear();

    if (new_pitch_lsb == pitch_lsb_state && new_pitch_msb == pitch_msb_state &&
            curr_sample_bank == sample_location->second.bank &&
            curr_sample_address == sample_location->second.ptr - 0x10) {
        bytes.push_back(CMD_SAMPLE_NEXT | FLAG_CMD);
    } else if (curr_sample_bank == sample_location->second.bank &&
            curr_sample_address == sample_location->second.ptr) {
        if (pitch_lsb_state != new_pitch_lsb) {
            bytes.push_back(0x1d | FLAG_CMD);
            bytes.push_back(new_pitch_lsb);
        }
        bytes.push_back(0x1e | FLAG_CMD);
        bytes.push_back(new_pitch_msb);
    } else {
        bytes.push_back(CMD_SAMPLE_START | FLAG_CMD);
        assert(sample_location->second.bank < 0x100);
        bytes.push_back(sample_location->second.bank);
        bytes.push_back(sample_location->second.ptr & 0xff);
        bytes.push_back(sample_location->second.ptr >> 8);
        bytes.push_back(new_pitch_lsb);
        bytes.push_back(new_pitch_msb);
    }

    pitch_lsb_state = new_pitch_lsb;
    pitch_msb_state = new_pitch_msb;
    curr_sample_bank = sample_location->second.bank;
    curr_sample_address = sample_location->second.ptr;
}

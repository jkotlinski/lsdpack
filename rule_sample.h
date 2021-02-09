#pragma once

#include "rule.h"

#include <map>
#include <vector>

#include "location.h"

class SampleRule : public Rule {
    private:
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

        size_t width() const { return 44; }

        void transform(std::deque<unsigned int>& bytes);
};

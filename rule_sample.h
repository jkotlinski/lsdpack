#include "rule.h"

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
        SampleRule() :
            sample_count(0),
            curr_sample_bank(-1),
            curr_sample_address(-1),
            pitch_lsb_state(-1),
            pitch_msb_state(-1) {
                write_location.bank = 1;
                write_location.ptr = 0x4000;
            }

        std::vector<unsigned char> get_samples() { return all_samples; }

        size_t width() const { return 44; }

        void transform(std::deque<unsigned int>& bytes) {
            const bool wav_write =
                bytes[0] == (0x25 | CMD_FLAG) &&
                bytes[4] == (0x30 | CMD_FLAG) &&
                bytes[42] == (0x25 | CMD_FLAG);

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
                bytes.push_back(SAMPLE_NEXT | CMD_FLAG);
            } else if (curr_sample_bank == sample_location->second.bank &&
                    curr_sample_address == sample_location->second.ptr) {
                if (pitch_lsb_state != new_pitch_lsb) {
                    bytes.push_back(0x1d | CMD_FLAG);
                    bytes.push_back(new_pitch_lsb);
                }
                bytes.push_back(0x1e | CMD_FLAG);
                bytes.push_back(new_pitch_msb);
            } else {
                bytes.push_back(SAMPLE | CMD_FLAG);
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
};

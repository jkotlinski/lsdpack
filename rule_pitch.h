#include "rule.h"

class PitchRule : public Rule {
    private:
        unsigned int pu0_lsb_state;
        unsigned int pu0_msb_state;
        unsigned int pu1_lsb_state;
        unsigned int pu1_msb_state;
        unsigned int wav_lsb_state;
        unsigned int wav_msb_state;

    public:
        PitchRule() :
            pu0_lsb_state(-1),
            pu0_msb_state(-1),
            pu1_lsb_state(-1),
            pu1_msb_state(-1),
            wav_lsb_state(-1),
            wav_msb_state(-1) {}

        size_t width() const { return 4; }

        void transform(std::deque<unsigned int>& bytes) {
            int cmd = 0;
            unsigned int new_lsb = bytes[1];
            unsigned int new_msb = bytes[3];
            bool trig = new_msb & 0x80;
            if (bytes[0] == (0x13 | CMD_FLAG) && bytes[2] == (0x14 | CMD_FLAG)) {
                if (new_msb == pu0_msb_state && !trig) {
                    // msb is redundant
                    cmd = (new_lsb == pu0_lsb_state)
                        ? 0 // lsb is redundant, too
                        : (0x13 | CMD_FLAG); // only set lsb
                } else {
                    cmd = PITCH_PU0 | CMD_FLAG;
                }
                pu0_msb_state = new_msb;
                pu0_lsb_state = new_lsb;
            } else if (bytes[0] == (0x18 | CMD_FLAG) && bytes[2] == (0x19 | CMD_FLAG)) {
                if (new_msb == pu1_msb_state && !trig) {
                    // msb is redundant
                    cmd = (new_lsb == pu1_lsb_state)
                        ? 0 // lsb is redundant, too
                        : (0x18 | CMD_FLAG); // only set lsb
                } else {
                    cmd = PITCH_PU1 | CMD_FLAG;
                }
                pu1_msb_state = new_lsb;
                pu1_lsb_state = new_msb;
            } else if (bytes[0] == (0x1d | CMD_FLAG) && bytes[2] == (0x1e | CMD_FLAG)) {
                if (new_msb == wav_msb_state && !trig) {
                    // msb is redundant
                    cmd = (new_lsb == wav_lsb_state)
                        ? 0 // lsb is redundant, too
                        : (0x1d | CMD_FLAG); // only set lsb
                } else {
                    cmd = PITCH_WAV | CMD_FLAG;
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

};

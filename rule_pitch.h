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
        PitchRule();

        size_t width() const { return 4; }

        void transform(std::deque<unsigned int>& bytes);
};

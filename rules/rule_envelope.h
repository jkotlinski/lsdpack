#include "rule.h"

class EnvelopeRule : public Rule {
    public:
        size_t window_size() const { return 15 * 2; }

        /* LSDj 8.8.0+ soft envelope problem:
         * To decrease volume on CGB, the byte 8 is written 15 times to
         * either of addresses 0xff12, 0xff17 or 0xff21.
         * To improve sound on DMG and reduce ROM/CPU usage, replace this
         * with commands AMP_DOWN_XXX
         */
        void transform(std::deque<unsigned int>& bytes);
};

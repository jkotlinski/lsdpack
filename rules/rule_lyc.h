#include "rule.h"

class LycRule : public Rule {
    public:
        size_t window_size() const { return 8; }

        // CMD:*:LYC => CMD|0x80:*
        void transform(std::deque<unsigned int>& bytes) {
            if (bytes[7] != (LYC | CMD_FLAG)) {
                return;
            }

            int i;
            for (i = 6; i >= 0; --i) {
                if (bytes[i] & CMD_FLAG) {
                    break;
                }
            }
            if (i == -1) {
                return;
            }

            if (bytes[i] == (LYC | CMD_FLAG)) {
                return;
            }
            if (bytes[i] & LYC_END_MASK) {
                return;
            }

            bytes[i] |= LYC_END_MASK;
            bytes.resize(7);
        }
};

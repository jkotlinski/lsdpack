#include "rule.h"

class LycRule : public Rule {
    public:
        size_t width() const { return 4; }

        // CMD:*:LYC => CMD|0x80:*
        void transform(std::deque<unsigned int>& bytes) {
            if (bytes[3] != (LYC | CMD_FLAG)) {
                return;
            }

            int i = -1;
            if (bytes[2] & CMD_FLAG) {
                i = 2;
            } else if (bytes[1] & CMD_FLAG) {
                i = 1;
            } else if (bytes[0] & CMD_FLAG) {
                i = 0;
            }
            if (i == -1) {
                return;
            }

            if (bytes[i] == (WAIT | CMD_FLAG)) {
                return;
            }
            if (bytes[i] == (LYC | CMD_FLAG)) {
                return;
            }

            bytes[i] |= LYC_END_MASK;
            bytes.resize(3);
        }
};

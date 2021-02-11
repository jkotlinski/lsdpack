#include "rule.h"

class LycRule : public Rule {
    public:
        size_t window_size() const override { return 8; }

        // CMD:*:LYC => CMD|0x80:*
        void transform(std::deque<unsigned int>& bytes) override {
            if (bytes[7] != (FLAG_CMD | CMD_END_TICK)) {
                return;
            }

            int i;
            for (i = 6; i >= 0; --i) {
                if (bytes[i] & FLAG_CMD) {
                    break;
                }
            }
            if (i == -1) {
                return;
            }

            if (bytes[i] == (FLAG_CMD | CMD_END_TICK)) {
                return;
            }
            if (bytes[i] & FLAG_END_TICK) {
                return;
            }

            bytes[i] |= FLAG_END_TICK;
            bytes.resize(7);
        }
};

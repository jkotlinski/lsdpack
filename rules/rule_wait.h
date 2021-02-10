#include "rule.h"

class WaitRule : public Rule {
    public:
        size_t window_size() const override { return 3; }

        void transform(std::deque<unsigned int>& bytes) override {
            if (bytes[0] == (LYC | CMD_FLAG) &&
                    bytes[1] == (LYC | CMD_FLAG) &&
                    bytes[2] == (LYC | CMD_FLAG)) {
                // LYC:LYC:LYC => WAIT:2
                bytes.clear();
                bytes.push_back(WAIT | CMD_FLAG);
                bytes.push_back(2);
            } else if (bytes[0] == (WAIT | CMD_FLAG) &&
                    bytes[1] != 0xff &&
                    bytes[2] == (LYC | CMD_FLAG)) {
                // WAIT:duration:LYC => WAIT:(duration+1)
                bytes.resize(2);
                ++bytes[1];
            }
        }
};

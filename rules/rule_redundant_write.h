#include "rule.h"

#include <cstring>

class RedundantWriteRule : public Rule {
    public:
        explicit RedundantWriteRule(unsigned int reg) : reg(reg), reg_state(-1) { }

        size_t window_size() const { return 2; }

        void transform(std::deque<unsigned int>& bytes) {
            if (bytes[0] != (reg | CMD_FLAG)) {
                return;
            }

            if (bytes[1] == reg_state) {
                bytes.clear();
            } else {
                reg_state = bytes[1];
            }
        }

    private:
        unsigned int reg;
        unsigned int reg_state;
};

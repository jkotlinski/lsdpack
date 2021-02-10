#include "rule.h"

class RepeatCommandRule : public Rule {
    public:
        size_t window_size() const override { return 64; }

        void transform(std::deque<unsigned int>& bytes) override;
};

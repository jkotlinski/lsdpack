#include "rule.h"

class InterruptedSampleRule : public Rule {
    public:
        size_t window_size() const { return 45; }

        // Fixes the situation where an interrupt happens while starting a wave
        // by moving the interrupt after the wave.
        void transform(std::deque<unsigned int>& bytes);
};

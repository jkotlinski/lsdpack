#include "rule_interrupted_sample.h"

#include <cassert>

void InterruptedSampleRule::transform(std::deque<unsigned int>& bytes) {
    const bool interrupted_sample =
        (bytes[0] == (0x25 | FLAG_CMD) && bytes[4] == (0x30 | FLAG_CMD) && bytes[43] == (0x25 | FLAG_CMD)) ||
        (bytes[0] == (0x25 | FLAG_CMD) && bytes[5] == (0x30 | FLAG_CMD) && bytes[43] == (0x25 | FLAG_CMD));

    if (!interrupted_sample) {
        return;
    }

    size_t i = 0;
    while (i < bytes.size()) {
        if (bytes[i] == (CMD_END_TICK | FLAG_CMD)) {
            break;
        }
        ++i;
    }
    assert(i != bytes.size());
    while (i < bytes.size() - 1) {
        bytes[i] = bytes[i + 1];
        ++i;
    }
    bytes[i] = (CMD_END_TICK | FLAG_CMD);
}

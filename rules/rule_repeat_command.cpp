#include "rule_repeat_command.h"

#include <cassert>

static void remove_duplicate_command(std::deque<unsigned int>& bytes, size_t i) {
    while (i < bytes.size() - 1) {
        bytes[i] = bytes[i + 1];
        ++i;
    }
    bytes.resize(bytes.size() - 1);
}

/* If the same command appears more than twice in a row:
 * - add REPEAT_MASK to the first command byte
 * - discard following repeated command bytes
 * - insert a repetition count byte immediately after the first command byte
 */
void RepeatCommandRule::transform(std::deque<unsigned int>& bytes) {
    if (!(bytes[0] & CMD_FLAG)) {
        return;
    }

    // Returns if the command does not appear at least three times in a row.
    if (!(bytes[0] & REPEAT_MASK)) {
        int duplicate_count = 0;
        for (size_t i = 1; i < bytes.size(); ++i) {
            if (!(bytes[i] & CMD_FLAG)) {
                continue;
            }
            if ((bytes[i] & ~REPEAT_MASK) == (bytes[0] & ~REPEAT_MASK)) {
                ++duplicate_count;
            } else {
                break;
            }
        }
        if (duplicate_count < 2) {
            return;
        }
    }

    while (true) {
        size_t i;

        // Finds the next duplicate command.
        for (i = 1; i < bytes.size(); ++i) {
            if (!(bytes[i] & CMD_FLAG)) {
                continue;
            }
            if ((bytes[i] & ~REPEAT_MASK) == (bytes[0] & ~REPEAT_MASK)) {
                break;
            } else {
                return;
            }
        }

        if (i == bytes.size()) {
            return; // No more duplicate command found.
        }

        if (bytes[0] & REPEAT_MASK) {
            // Repeat flag already set. Increase repetition count.
            if (bytes[1] != 0xff) {
                ++bytes[1];
                remove_duplicate_command(bytes, i);
            }
            return;
        }

        remove_duplicate_command(bytes, i);

        // Adds the repeat flag with one repetition.
        bytes[0] |= REPEAT_MASK;
        bytes.insert(bytes.begin() + 1, 1);
    }
}

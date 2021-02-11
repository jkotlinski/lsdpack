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
 * - add FLAG_REPEAT to the first command byte
 * - discard following repeated command bytes
 * - insert a repetition count byte immediately after the first command byte
 */
void RepeatCommandRule::transform(std::deque<unsigned int>& bytes) {
    if (!(bytes[0] & FLAG_CMD)) {
        return;
    }

    // Returns if the command does not appear at least three times in a row.
    if (!(bytes[0] & FLAG_REPEAT)) {
        int duplicate_count = 0;
        for (size_t i = 1; i < bytes.size(); ++i) {
            if (!(bytes[i] & FLAG_CMD)) {
                continue;
            }
            if ((bytes[i] & ~FLAG_REPEAT) == (bytes[0] & ~FLAG_REPEAT)) {
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
            if (!(bytes[i] & FLAG_CMD)) {
                continue;
            }
            if ((bytes[i] & ~FLAG_REPEAT) == (bytes[0] & ~FLAG_REPEAT)) {
                break;
            } else {
                return;
            }
        }

        if (i == bytes.size()) {
            return; // No more duplicate command found.
        }

        if (bytes[0] & FLAG_REPEAT) {
            // Repeat flag already set. Increase repetition count.
            if (bytes[1] != 0xff) {
                ++bytes[1];
                remove_duplicate_command(bytes, i);
            }
            return;
        }

        remove_duplicate_command(bytes, i);

        // Adds the repeat flag with one repetition.
        bytes[0] |= FLAG_REPEAT;
        bytes.insert(bytes.begin() + 1, 1);
    }
}

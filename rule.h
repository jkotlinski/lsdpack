#pragma once

#include <deque>

enum Cmds {
    LYC,
    SAMPLE,
    STOP,
    NEXT_BANK,
    AMP_DOWN_PU0,
    AMP_DOWN_PU1,
    AMP_DOWN_NOI,
    PITCH_PU0,
    PITCH_PU1,
    PITCH_WAV,
    WAIT,
    SAMPLE_NEXT
};

enum Defines {
    LYC_END_MASK = 0x80,
    SONG_START = 0x100,
    CMD_FLAG = 0x200
};

class Rule {
    public:
        virtual size_t width() = 0;
        virtual void transform(std::deque<unsigned int>& bytes) = 0;
};

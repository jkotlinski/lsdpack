#pragma once

#include <cstdio>
#include <deque>

enum Cmds {
    CMD_END_TICK,
    CMD_SAMPLE_START,
    CMD_SONG_STOP,
    CMD_NEXT_BANK,
    CMD_AMP_DEC_PU0,
    CMD_AMP_DEC_PU1,
    CMD_AMP_DEC_NOI,
    CMD_PITCH_PU0,
    CMD_PITCH_PU1,
    CMD_PITCH_WAV,
    CMD_SAMPLE_NEXT
};

enum Flags {
    FLAG_END_TICK   = 0x80,
    FLAG_REPEAT     = 0x40,
    FLAG_SONG_START = 0x100,
    FLAG_CMD        = 0x200
};

class Rule {
    public:
        virtual size_t window_size() const = 0;
        virtual void transform(std::deque<unsigned int>& bytes) = 0;
};

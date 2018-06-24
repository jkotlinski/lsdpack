#pragma once

#include "inputgetter.h"

enum Button { A     = 0x01, B    = 0x02, SELECT = 0x04, START = 0x08,
              RIGHT = 0x10, LEFT = 0x20, UP     = 0x40, DOWN  = 0x80 };

struct Input : public gambatte::InputGetter {
    Input() : pressed(0) {}

	virtual unsigned operator()() {
        return pressed;
    }

    void press(unsigned button) {
        pressed = button;
    }

    unsigned pressed;
};

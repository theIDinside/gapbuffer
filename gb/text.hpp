//
// Created by 46769 on 2021-01-21.
//

#pragma once
#include "gap_buffer.hpp"

// The actual interface to use the GapBuffer via, in CXGledit

class Text {
public:
    Text();
private:
    GapBuffer buffer;
};
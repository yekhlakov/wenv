#pragma once

#include <vector>
#include "Character.h"

namespace Wenv::Display {

struct Display {
    std::vector<std::vector<Character>> data;

    void resize(size_t width, size_t height);
};

} // namespace Wenv::Display

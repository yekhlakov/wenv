#include "Display.h"

namespace Wenv::Display {

void Display::resize(size_t width, size_t height) {
    data.resize(height);
    for (size_t y = 0; y < height; ++y) {
        size_t old_width = data[y].size();
        data[y].resize(width);
        if (width > old_width) {
            for (size_t x = old_width; x < width; ++x) {
                Character space;
                space.set(L' ', 0, 0);
                data[y][x] = space;
            }
        }
    }
}

} // namespace Wenv::Display

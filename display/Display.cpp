#include "Display.h"
#include <random>

namespace Wenv::Display {

void Display::resize(size_t width, size_t height) {
    data.resize(height);
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> ascii_dist(0x0020, 0x007E);
    std::uniform_int_distribution<int> cyrillic_dist(0x0400, 0x04FF);
    std::uniform_int_distribution<int> hanzi_dist(0x4E00, 0x9FFF);
    std::uniform_int_distribution<int> region_dist(0, 2);
    std::uniform_int_distribution<int> color_dist(0, 0xFFFFFF);

    for (size_t y = 0; y < height; ++y) {
        data[y].resize(width);
        for (size_t x = 0; x < width; ++x) {
            wchar_t ch;
            int region = region_dist(gen);
            if (region == 0) ch = static_cast<wchar_t>(ascii_dist(gen));
            else if (region == 1) ch = static_cast<wchar_t>(cyrillic_dist(gen));
            else ch = static_cast<wchar_t>(hanzi_dist(gen));

            data[y][x].set(ch, color_dist(gen), 0);
        }
    }
}

} // namespace Wenv::Display

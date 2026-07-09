#include "Display.h"
#include <random>

namespace Wenv::Display
{

void Display::resize(size_t width, size_t height)
{
    data.resize(height);
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> ascii_dist(0x0020, 0x007E);
    std::uniform_int_distribution<int> cyrillic_dist(0x0400, 0x045F);
    std::uniform_int_distribution<int> region_dist(0, 1);
    std::uniform_int_distribution<int> palette_dist(0, 16);
    //std::uniform_int_distribution<int> color_dist(0, 0xFFFFFF);

    for (size_t y = 0; y < height; ++y) {
        data[y].resize(width);
        for (size_t x = 0; x < width; ++x) {
            wchar_t ch;
            //int region = region_dist(gen);
            //if (region == 0) 
            //ch = static_cast<wchar_t>(ascii_dist(gen));
            //else 
            ch = static_cast<wchar_t>(cyrillic_dist(gen));

            data[y][x].set(ch, palette_dist(gen));
        }
    }

    draw_box (4, 4, width - 8, height - 8, 2);
    draw_box (8, 8, width - 16, height - 16, 1);
}



// Set the palette color (or real color) for subsequent output operations
void Display::set_color (int pc, int fg, int bg)
{
	current_palette_color = pc;
	current_foreground_color = fg;
	current_background_color = bg;
}


} // namespace Wenv::Display

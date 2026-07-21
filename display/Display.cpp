#include "Display.h"
#include "../Layout/Grid.h"
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



	::Wenv::Layout::Grid grid;


	grid.add_row (0, 0, 50.0);
	grid.add_row (0, 0, 25.0);
    grid.add_row (0, 0, 25.0);
	grid.add_column (0, 0, 33.0);
	grid.add_column (0, 0, 17.0);
    grid.add_column (0, 0, 50.0);
	grid.is_exclusive = false;

    grid.add_block ({ 0, 0, 2, 1 }, 1);
    grid.add_block ({ 2, 0, 1, 1 }, 0);
    grid.add_block ({ 0, 1, 1, 2 }, 2);
    grid.add_block ({ 1, 1, 2, 1 }, 1);
    grid.add_block ({ 1, 2, 2, 1 }, 2);

    grid.bake ({ 0, 0, (int) width, (int) height });

    for (auto &b : grid.blocks)
    {
        draw_block (b);
    }

    /*
	auto b = grid.get_block_dimensions ({0, 0, (int) width, (int) height}, {0, 0, 2, 1});
	draw_box (b.x, b.y, b.width, b.height, 1);
    b = grid.get_block_dimensions ({ 0, 0, (int) width, (int) height }, { 2, 0, 1, 1 });
    draw_box (b.x, b.y, b.width, b.height, 0);

    b = grid.get_block_dimensions ({ 0, 0, (int) width, (int) height }, { 0, 1, 1, 2 });
    draw_box (b.x, b.y, b.width, b.height, 2);

    b = grid.get_block_dimensions ({ 0, 0, (int) width, (int) height }, { 1, 1, 2, 1 });
    draw_box (b.x, b.y, b.width, b.height, 1);
    b = grid.get_block_dimensions ({ 0, 0, (int) width, (int) height }, { 1, 2, 2, 1 });
    draw_box (b.x, b.y, b.width, b.height, 2);
    */

    //draw_box (4, 4, width - 8, height - 8, 2);
    //draw_box (8, 8, width - 16, height - 16, 1);
}



// Set the palette color (or real color) for subsequent output operations
void Display::set_color (int pc, int fg, int bg)
{
	current_palette_color = pc;
	current_foreground_color = fg;
	current_background_color = bg;
}


} // namespace Wenv::Display

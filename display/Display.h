#pragma once

#include <string>
#include <vector>
#include "Character.h"

namespace Wenv::Layout
{
struct Layout;
struct Grid;
struct Block;
}

namespace Wenv::Display {

struct Display
{
    std::vector<std::vector<Character>> data;

    int current_palette_color = 0;
    int current_foreground_color = 0;
    int current_background_color = 0;

	::Wenv::Layout::Layout *layout;

    // Resize the display
    void resize (size_t width, size_t height);


	// OUTPUT

	// Set the palette color (or real color) for subsequent output operations
 	void set_color (int pc = 0, int fg = 0, int bg = 0);

	// Print character to specified position
	void print_char (size_t pos, size_t ln, wchar_t ch);

	// Print line left to right starting from specified position
	void print_line (size_t pos, size_t ln, const std::wstring & s);

	// Print line top to bottom starting from specified position
	void print_line_v (size_t pos, size_t ln, const std::wstring & s);

	// Draw a grid (recursively)
	void draw_grid (::Wenv::Layout::Grid &grid);

	// Draw a grid block (using its boundary strings)
	void draw_block (::Wenv::Layout::Block &b);

	// Draw rectangular box with constant border
	void draw_box (size_t pos, size_t ln, size_t w, size_t h, int btype);
};

} // namespace Wenv::Display

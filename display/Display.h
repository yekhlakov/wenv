#pragma once

#include <string>
#include <vector>
#include "Character.h"
#include "..\Types.h"

namespace Wenv::Layout
{
struct Layout;
struct Grid;
struct Block;
}

namespace Wenv::Display {

struct Display
{
	inline static const int PF_LEFT = 0;
	inline static const int PF_CENTER = 1;
	inline static const int PF_RIGHT = 2;
	inline static const int PF_TOP = 0;
	inline static const int PF_VCENTER = 4;
	inline static const int PF_BOTTOM = 8;
	inline static const int PF_CLIP = 0;
	inline static const int PF_NOCLIP = 16;
	inline static const int PF_ERASE_BACKGROUND = 32;

    std::vector<std::vector<Character>> data;

    int current_palette_color = 0;
    int current_foreground_color = 0;
    int current_background_color = 0;

	::Wenv::Layout::Grid * grid = nullptr;

	~Display ();

    // Resize the display
    void resize (size_t width, size_t height);


	// OUTPUT

	// Set the palette color (or real color) for subsequent output operations
 	void set_color (int pc = 0, int fg = 0, int bg = 0);

	// Print character to specified position
	void print_char (size_t pos, size_t ln, wchar_t ch);

	// Print line left to right starting from specified position
	void print_line (size_t pos, size_t ln, const std::wstring & s);

	// Extended printing
	void print_line (Rect container, const std::wstring &s, int flags = 0);

	// Print line top to bottom starting from specified position
	void print_line_v (size_t pos, size_t ln, const std::wstring & s);

	// Draw a grid (recursively)
	void draw_grid (::Wenv::Layout::Grid &grid);

	// Draw a grid block (using its boundary strings)
	void draw_block_boundary (::Wenv::Layout::Block &b);

	// Draw rectangular box with constant border
	void draw_box (size_t pos, size_t ln, size_t w, size_t h, int btype);
};

} // namespace Wenv::Display

#include "Display.h"
#include "../maxy/strings.h"
#include "../layout/Grid.h"

namespace Wenv::Display
{

// Print character to specified position
void Display::print_char (size_t pos, size_t ln, wchar_t ch)
{
	if (ln >= data.size ()) return;

	if (pos >= data[ln].size ()) return;

	data[ln][pos].set (ch, current_palette_color, current_foreground_color, current_background_color);
}

// Print line left to right starting from specified position
void Display::print_line (size_t pos, size_t ln, const std::wstring & s)
{
	if (ln >= data.size ()) return;
	if (s.size () == 0) return;

	auto i = 0;

	while (pos < data[ln].size () && i < s.size ())
	{
		data[ln][pos].set (s[i], current_palette_color, current_foreground_color, current_background_color);
		pos++;
		i++;		
	}
}

// Print line top to bottom starting from specified position
void Display::print_line_v (size_t pos, size_t ln, const std::wstring & s)
{
	if (s.size () == 0) return;

	// Hacky way to check the coords
	if (pos >= data[0].size ()) return;

	auto i = 0;

	while (ln < data.size () && i < s.size ())
	{
		data[ln][pos].set (s[i], current_palette_color, current_foreground_color, current_background_color);
		ln++;
		i++;		
	}
}

void Display::draw_block (::Wenv::Layout::Block &b)
{
	print_line (b.container_dimensions.x, b.container_dimensions.y, b.top_boundary);
	print_line_v (b.container_dimensions.x, b.container_dimensions.y + 1, b.left_boundary);
	print_line_v (b.container_dimensions.x + b.container_dimensions.width - 1, b.container_dimensions.y + 1, b.right_boundary);
	print_line (b.container_dimensions.x, b.container_dimensions.y + b.container_dimensions.height - 1, b.bottom_boundary);
}

// Draw rectangular box with constant border
void Display::draw_box (size_t pos, size_t ln, size_t w, size_t h, int btype)
{
	if (w < 2 || h < 2)
	{
		return;
	}

	std::wstring element_base[][6] = {
		{
			maxy::strings::utf8towchar (" "),
			maxy::strings::utf8towchar (" "),
			maxy::strings::utf8towchar (" "),
			maxy::strings::utf8towchar (" "),
			maxy::strings::utf8towchar (" "),
			maxy::strings::utf8towchar (" ")
		},
		{
			maxy::strings::utf8towchar ("─"),
			maxy::strings::utf8towchar ("│"),
			maxy::strings::utf8towchar ("┌"),
			maxy::strings::utf8towchar ("┐"),
			maxy::strings::utf8towchar ("└"),
			maxy::strings::utf8towchar ("┘")
		},
		{
			maxy::strings::utf8towchar ("═"),
			maxy::strings::utf8towchar ("║"),
			maxy::strings::utf8towchar ("╔"),
			maxy::strings::utf8towchar ("╗"),
			maxy::strings::utf8towchar ("╚"),
			maxy::strings::utf8towchar ("╝")
		}
	}; 
	
	auto &elements = element_base[btype];

	std::wstring top = elements[2] + std::wstring (w - 2, elements[0][0]) + elements[3];
	std::wstring bottom = elements[4] + std::wstring (w - 2, elements[0][0]) +elements[5];
	std::wstring vert (h - 2, elements[1][0]);

	print_line (pos, ln, top);
	print_line_v (pos, ln + 1, vert);
	print_line_v (pos + w - 1, ln + 1, vert);
	print_line (pos, ln + h - 1, bottom);
}
	

}

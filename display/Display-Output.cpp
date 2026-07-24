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

void Display::print_line (Rect container, const std::wstring &s, int flags)
{
	int l = (int) s.size ();
	int dw = (int) data[0].size ();
	int dh = (int) data.size ();

	if
		(
			container.width < 1 || container.height < 1 ||
			container.x > dw ||
			container.y > dh ||
			container.x + container.width < 0 ||
			container.y + container.height < 0 ||
			l == 0
		)
	{
		// Container is either collapsed or totally outside the display, or the line is empty - do nothing
		return;
	}

	// Compute the line position
	int x =
		(flags & PF_CENTER)
		? container.x + (container.width - l) / 2
		: (flags & PF_RIGHT)
		? container.x + container.width - l - 1
		: container.x;

	int y =
		(flags & PF_VCENTER)
		? container.y + container.height / 2
		: (flags & PF_BOTTOM)
		? container.y + container.height - 1
		: container.y;

	if (flags & PF_ERASE_BACKGROUND)
	{
		for (auto cy = std::max (0, container.y); cy < std::min (dh, container.y + container.height); cy++)
		{
			for (auto cx = std::max (0, container.x); cx < std::min (dw, container.x + container.width); cx++)
			{
				if (cy == y && cx == x)
				{
					// Skip the whole line
					cx += l - 1;
					continue;
				}

				print_char (cx, cy, L' ');
			}
		}
	}

	if (x < - l || y < 0 || x >= dw || y >= dh)
	{
		// The line is completely outside the display - do nothing
		return;
	}

	print_line (x, y, s);
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

void Display::draw_block_boundary (::Wenv::Layout::Block &b)
{
	if (b.btype < 0)
	{
		// Borderless blocks
		return;
	}

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

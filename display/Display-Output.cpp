#include "Display.h"
#include "Palette.h"
#include "../maxy/strings.h"
#include "../layout/Grid.h"


namespace Wenv::Display
{

Display *Display::with_palette (Palette *p)
{
	current_palette = p;

	return this;
}

Display *Display::with_color (const std::string &n)
{
	if (current_palette != nullptr)
	{
		auto e = current_palette->get_entry (n);

		current_palette_color = -1;
		current_foreground_color = e.foreground_color;
		current_background_color = e.background_color;
	}

	return this;
}

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
			container.y + container.height < 0
		)
	{
		// Container is either collapsed or totally outside the display - do nothing
		return;
	}

	// Compute the line position
	int x =
		(flags & PF_CENTER)
		? container.x + (container.width - l) / 2
		: (flags & PF_RIGHT)
		? container.x + container.width - l
		: container.x;

	int y =
		(flags & PF_VCENTER)
		? container.y + container.height / 2
		: (flags & PF_BOTTOM)
		? container.y + container.height - 1
		: container.y;


	auto container_begin_x = std::max (0, container.x);
	auto container_end_x = std::min (dw, container.x + container.width);

	if (flags & PF_ERASE_BACKGROUND)
	{
		for (auto cy = std::max (0, container.y); cy < std::min (dh, container.y + container.height); cy++)
		{
			for (auto cx = container_begin_x; cx < container_end_x; cx++)
			{
				if (cy == y && cx == x && l > 0)
				{
					// Skip the whole line
					cx += l - 1 ;

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

	int begin_s = 0, end_s = (int) s.size ();

	if (x < container_begin_x)
	{
		begin_s = container_begin_x - x;
	}

	if (x + l >= container_end_x)
	{
		end_s = container_end_x - x;
	}

	if (begin_s < end_s)
	{
		print_line (x + begin_s, y, std::wstring (s, begin_s, end_s));
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

void Display::draw_block_boundary (::Wenv::Layout::Block &b, const std::string & path)
{
	if (b.btype < 0)
	{
		// Borderless blocks
		return;
	}

	print_line (b.instances[path].container_dimensions.x, b.instances[path].container_dimensions.y, b.instances[path].top_boundary);
	print_line_v (b.instances[path].container_dimensions.x, b.instances[path].container_dimensions.y + 1, b.instances[path].left_boundary);
	print_line_v (b.instances[path].container_dimensions.x + b.instances[path].container_dimensions.width - 1, b.instances[path].container_dimensions.y + 1, b.instances[path].right_boundary);
	print_line (b.instances[path].container_dimensions.x, b.instances[path].container_dimensions.y + b.instances[path].container_dimensions.height - 1, b.instances[path].bottom_boundary);
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

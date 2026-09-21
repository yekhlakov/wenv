#include <algorithm>
#include <format>
#include <utility>
#include "Display.h"
#include "Palette.h"
#include "../apps/App.h"
#include "../maxy/strings.h"
#include "../layout/Grid.h"


namespace Wenv::Display
{

// Set the palette used by the subsequent output operations
Display *Display::with_palette (Palette *p)
{
	current_palette = p;

	return this;
}

// Set the foreground and background drawing colors from the named palette
// entry; the highlight variant of the entry is used when requested
Display *Display::with_color (const std::string &n, bool is_highlight)
{
	if (current_palette != nullptr)
	{
		auto e = current_palette->get_entry (n);

		current_palette_color = -1;

		if (is_highlight)
		{
			current_foreground_color = e.highlight_foreground_color;
			current_background_color = e.highlight_background_color;
		}
		else
		{
			current_foreground_color = e.foreground_color;
			current_background_color = e.background_color;
		}
	}

	return this;
}

// Set the palette color (or real color) for subsequent output operations
void Display::set_color (int pc, int fg, int bg)
{
	current_palette_color = pc;
	current_foreground_color = fg;
	current_background_color = bg;
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

// Print a line inside the given container, honoring the alignment and erasing
// flags; the parts of the line outside the container and the display are clipped
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

// Compute the dimensions of the minimal rectangle (in characters) this text can be printed
// into, with word wrapping. Words are placed left to right; when a word does not fit on the
// current row, the algorithm either widens the rectangle or wraps the word to a new row,
// picking the option with the smaller (penalized) area.
Rect Display::get_min_rectangle (const std::wstring &text, int min_width, int min_height, int max_width, int max_height, std::vector<std::wstring> *out_lines)
{
	// Whitespace separator (tabs, spaces and, for safety, carriage returns)
	auto is_separator = [] (wchar_t ch)
	{
		return ch == L' ' || ch == L'\t' || ch == L'\r';
	};

	// Split the text into lines (hard newlines) and trim each of them
	std::vector<std::wstring> lines;
	{
		size_t pos = 0;
		while (true)
		{
			size_t next = text.find (L'\n', pos);
			if (next == std::wstring::npos)
			{
				next = text.length ();
			}

			auto line = text.substr (pos, next - pos);
			size_t first = 0;
			size_t last = line.length ();
			while (first < last && is_separator (line[first]))
			{
				++first;
			}
			while (last > first && is_separator (line[last - 1]))
			{
				--last;
			}
			lines.push_back (line.substr (first, last - first));

			if (next == text.length ())
			{
				break;
			}
			pos = next + 1;
		}
	}

	// Split the text into words (runs of non-separator characters), remembering the line
	// each word belongs to: a hard newline forces a wrap no matter how much space is left
	std::vector<std::pair<std::wstring, int>> words;
	for (size_t li = 0; li < lines.size (); ++li)
	{
		size_t pos = 0;
		auto &line = lines[li];
		while (pos < line.length ())
		{
			while (pos < line.length () && is_separator (line[pos]))
			{
				++pos;
			}
			size_t first = pos;
			while (pos < line.length () && !is_separator (line[pos]))
			{
				++pos;
			}
			if (pos > first)
			{
				words.push_back ({ line.substr (first, pos - first), (int) li });
			}
		}
	}

	// The width cannot be smaller than the longest word; the height cannot be smaller
	// than the number of lines (nor the caller-provided minimums)
	int width = min_width;
	int height = min_height;
	for (auto &[word, li] : words)
	{
		width = (std::max) (width, (int) word.length ());
	}
	height = (std::max) (height, (int) lines.size ());

	// Area of the rectangle with a penalty for exceeding the caller-provided limits: the
	// excess portion of the rectangle is added to the area
	auto penalized_area = [max_width, max_height] (int w, int h)
	{
		long long area = (long long) w * h;

		if (w > max_width)
		{
			area += (long long) (w - max_width) * h;
		}
		if (h > max_height)
		{
			area += (long long) (h - max_height) * w;
		}
		return area;
	};

	// Place the words one by one, remembering the row each word lands on
	int row = 0;
	int col = 0;
	std::vector<std::wstring> wrapped;
	for (size_t idx = 0; idx < words.size (); ++idx)
	{
		auto &[word, li] = words[idx];
		int len = (int) word.length ();

		// Hard newlines before this word force wraps; empty lines in between produce
		// empty rows
		if (idx == 0)
		{
			row = li;
			col = 0;
		}
		else if (words[idx - 1].second != li)
		{
			row += li - words[idx - 1].second;
			col = 0;
			height = (std::max) (height, row + 1);
		}

		// The word fits on the current row without modifying the width
		if (col + len <= width)
		{
			col += len + 1;
		}
		else
		{
			// Option 1: widen the rectangle to keep the word on the current row
			int new_width = col + len;
			long long area1 = penalized_area (new_width, height);

			// Option 2: wrap the word to a new row
			int new_height = (std::max) (height, row + 2);
			long long area2 = penalized_area (width, new_height);

			if (area1 <= area2)
			{
				width = new_width;
				col += len + 1;
			}
			else
			{
				height = new_height;
				row += 1;
				col = len;
			}
		}

		// Record the word in its output row: consecutive words on a row are always
		// separated by a single space
		if (out_lines != nullptr)
		{
			while ((int) wrapped.size () <= row)
			{
				wrapped.push_back (L"");
			}
			if (!wrapped[row].empty ())
			{
				wrapped[row] += L' ';
			}
			wrapped[row] += word;
		}
	}

	// Pad the remaining (empty) rows to match the final height
	if (out_lines != nullptr)
	{
		while ((int) wrapped.size () < height)
		{
			wrapped.push_back (L"");
		}
		*out_lines = std::move (wrapped);
	}

	return { 0, 0, width, height };
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

// Draw a grid recursively: all its boundaries in the given palette color
// (highlight variant if requested), then the contents of its app blocks
void Display::draw_grid (::Wenv::Layout::Grid &grid, std::string path, ::Wenv::Context * ctx, const std::string &color, bool highlight)
{
	// Fall back to the grid own context
	if (ctx == nullptr)
	{
		ctx = grid.context;
	}

	// Draw all the boundaries in the given color so the ambient drawing colors
	// of the display cannot bleed into the borders
	with_color (color, highlight);

	// First draw all boundaries
	int bnum = 0;
	for (auto &b : grid.blocks)
	{
		auto bpath = std::format ("{}.{}", path, bnum++);
		draw_block_boundary (b, bpath, color, highlight);
		if (b.grid != nullptr)
		{
			// If the block has nested grid, recurse
			draw_grid (*b.grid, bpath, b.get_context (ctx), color, highlight);
		}
	}

	// Then draw app conents because the conent may overwrite some boundaries
	bnum = 0;
	for (auto &b : grid.blocks)
	{
		auto bpath = std::format ("{}.{}", path, bnum++);
		if (b.app != nullptr)
		{
			// Otherwise if the block has an attached app, ask the app to draw its contents
			b.app->with_context (b.get_context (ctx))
				->draw (*this, bpath, b.get_client_dimensions (bpath));
		}
	}
}

// Draw the boundary of a grid block (using its boundary strings) in the given
// palette color, ignoring the ambient drawing colors
void Display::draw_block_boundary (::Wenv::Layout::Block &b, const std::string & path, const std::string &color, bool highlight)
{
	if (b.btype < 0)
	{
		// Borderless blocks
		return;
	}

	// Draw the boundary in its own color so the ambient drawing colors of the
	// display cannot bleed into the borders
	with_color (color, highlight);

	print_line (b.instances[path].container_dimensions.x, b.instances[path].container_dimensions.y, b.instances[path].top_boundary);
	print_line_v (b.instances[path].container_dimensions.x, b.instances[path].container_dimensions.y + 1, b.instances[path].left_boundary);
	print_line_v (b.instances[path].container_dimensions.x + b.instances[path].container_dimensions.width - 1, b.instances[path].container_dimensions.y + 1, b.instances[path].right_boundary);
	print_line (b.instances[path].container_dimensions.x, b.instances[path].container_dimensions.y + b.instances[path].container_dimensions.height - 1, b.instances[path].bottom_boundary);
}

// Redraw grid boundaries that cross the given display row within the given
// horizontal span (recursively). App contents are not redrawn; use this to
// restore borders that a title was drawn over. The boundaries are drawn in the
// given palette color, ignoring the ambient drawing colors
void Display::redraw_boundaries_on_row (::Wenv::Layout::Grid &grid, std::string path, int y, int x_begin, int x_end, const std::string &color)
{
	int bnum = 0;
	for (auto &b : grid.blocks)
	{
		auto bpath = std::format ("{}.{}", path, bnum++);
		auto &c = b.instances[bpath].container_dimensions;

		// Skip blocks that do not cross the row within the given span
		if (c.y > y || y >= c.y + c.height || c.x >= x_end || x_begin >= c.x + c.width)
		{
			continue;
		}

		draw_block_boundary (b, bpath, color);

		if (b.grid != nullptr)
		{
			redraw_boundaries_on_row (*b.grid, bpath, y, x_begin, x_end, color);
		}
	}
}

// Redraw the boundaries of the root grid that cross the given display row
// within the given horizontal span
void Display::redraw_boundaries_on_row (int y, int x_begin, int x_end, const std::string &color)
{
	if (grid != nullptr)
	{
		redraw_boundaries_on_row (*grid, "root", y, x_begin, x_end, color);
	}
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
#include <algorithm>
#include <format>
#include <random>
#include <utility>
#include "Display.h"
#include "Window.h"
#include "../Layout/Grid.h"
#include "../apps/FuncMenu/FuncMenu.h"
#include "../Context.h"

namespace Wenv::Display
{

Display::Display (const std::wstring &n) :
    name { n }
{
   
}

Display::~Display ()
{
    if (grid != nullptr)
    {
        delete grid;
    }
}

void Display::resize (size_t width, size_t height)
{
    data.resize (height);
    std::mt19937 gen (std::random_device {}());
    std::uniform_int_distribution<int> ascii_dist (0x0020, 0x007E);
    std::uniform_int_distribution<int> cyrillic_dist (0x0400, 0x045F);
    std::uniform_int_distribution<int> region_dist (0, 1);
    std::uniform_int_distribution<int> palette_dist (0, 16);
    //std::uniform_int_distribution<int> color_dist(0, 0xFFFFFF);


    for (size_t y = 0; y < height; ++y) {
        data[y].resize (width);
        for (size_t x = 0; x < width; ++x) {
            wchar_t ch;
            //int region = region_dist(gen);
            //if (region == 0) 
            //ch = static_cast<wchar_t>(ascii_dist(gen));
            //else 
            ch = static_cast<wchar_t> (cyrillic_dist (gen));

            data[y][x].set (ch, palette_dist (gen));
        }
    }

    // Char buffer to hold boundary texts
    std::vector<std::vector<wchar_t>> buffer (height, std::vector<wchar_t> (width + 1, L' '));

    grid->bake ({ 0, 0, (int) width, (int) height }, buffer, "root");

    draw_grid (*grid, "root");
}

// Compute the dimensions of the minimal rectangle (in characters) this text can be printed
// into, with word wrapping. Words are placed left to right; when a word does not fit on the
// current row, the algorithm either widens the rectangle or wraps the word to a new row,
// picking the option with the smaller (penalized) area.
Rect Display::get_min_rectangle (const std::wstring &text, int min_width, int min_height, int max_width, int max_height)
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

	// Place the words one by one
	int row = 0;
	int col = 0;
	for (size_t idx = 0; idx < words.size (); ++idx)
	{
		auto &[word, li] = words[idx];
		int len = (int) word.length ();

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

		// A hard newline before the next word forces a wrap
		if (idx + 1 < words.size () && words[idx + 1].second != li)
		{
			row += 1;
			col = 0;
			height = (std::max) (height, row + 1);
		}
	}

	return { 0, 0, width, height };
}

void Display::draw_grid (::Wenv::Layout::Grid &grid, std::string path, ::Wenv::Context * ctx)
{
	// Fall back to the grid own context
	if (ctx == nullptr)
	{
		ctx = grid.context;
	}

	// First draw all boundaries
    int bnum = 0;
    for (auto &b : grid.blocks)
    {
        auto bpath = std::format ("{}.{}", path, bnum++);
        draw_block_boundary (b, bpath);
        if (b.grid != nullptr)
        {
            // If the block has nested grid, recurse
            draw_grid (*b.grid, bpath, b.get_context (ctx));
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

// Redraw the boundaries of those grid blocks that intersect the given display row
// within the given horizontal span; recurse into nested grids
void Display::redraw_boundaries_on_row (::Wenv::Layout::Grid &grid, std::string path, int y, int x_begin, int x_end)
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

		draw_block_boundary (b, bpath);

		if (b.grid != nullptr)
		{
			redraw_boundaries_on_row (*b.grid, bpath, y, x_begin, x_end);
		}
	}
}

void Display::redraw_boundaries_on_row (int y, int x_begin, int x_end)
{
	if (grid != nullptr)
	{
		redraw_boundaries_on_row (*grid, "root", y, x_begin, x_end);
	}
}

// Set the palette color (or real color) for subsequent output operations
void Display::set_color (int pc, int fg, int bg)
{
	current_palette_color = pc;
	current_foreground_color = fg;
	current_background_color = bg;
}


::Wenv::Apps::App *Display::add_app (::Wenv::Apps::App *a)
{
    all_apps.push_back (a);
    if (a->wants_all_keypresses ())
    {
        listening_apps.push_back (a);
    }

    return a;
}


::Wenv::Context *Display::add_context (::Wenv::Context *c)
{
    contexts[c->get_name ()] = c;

    return c;
}

::Wenv::Context * Display::get_context (const std::string & n)
{
    if (contexts.find (n) == contexts.end ())
    {
        return nullptr;
    }

    return contexts[n];
}

::Wenv::Context *Display::get_persistent_context ()
{
    if (window == nullptr)
    {
        return nullptr;
    }

    return window->persistent_context;
}

bool Display::get_key_state (int key) const
{
	if (window == nullptr)
	{
		return false;
	}

	return window->get_key_state (key);
}

} // namespace Wenv::Display

#pragma once

#include <vector>
#include "Character.h"

namespace Wenv::Display
{

struct Palette
{
	struct Entry
	{
		int foreground_color;
		int background_color;

		Entry (int fg = 0x00808080, int bg = 0);
	};

	std::vector<Entry> colors;

	Palette ();
	Entry get_entry (size_t n);
};

} // namespace Wenv::Display

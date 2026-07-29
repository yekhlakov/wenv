#pragma once

#include <string>
#include <unordered_map>
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
	std::unordered_map<std::string, int> named_colors;

	Palette ();
	Entry get_entry (size_t n);
	Entry get_entry (const std::string &n);

	inline static const char *Default_color = "default";
	inline static const char *Highlight_color = "highlight";
	inline static const char *Active_element_color = "active";
};

} // namespace Wenv::Display

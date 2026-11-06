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
	int highlight_foreground_color;
	int highlight_background_color;

	// The variant used for the characters covered by a selection: the
	// foreground of the regular variant and the background of the
	// highlighted one, dimmed halfway down
	int selected_foreground_color;
	int selected_background_color;

	Entry (int fg = 0x00808080, int bg = 0, int hfg = 0, int hbg = 0, int sfg = 0, int sbg = 0);
};

	// The color variant of an entry the output operations are drawn with
	inline static const int Variant_regular = 0;
	inline static const int Variant_highlight = 1;
	inline static const int Variant_selected = 2;

	std::vector<Entry> colors;
	std::unordered_map<std::string, int> named_colors;

	Palette ();
	Entry get_entry (size_t n);
	Entry get_entry (const std::string &n);

	inline static const char *Default_color = "default";
	inline static const char *Active_element_color = "active";
	inline static const char *Dark_element_color = "dark";
	inline static const char *Quote_element_color = "quote";
	inline static const char *Number_element_color = "number";
	inline static const char *Alter_element_color = "alter";
	inline static const char *Warning_element_color = "warning";
};

} // namespace Wenv::Display

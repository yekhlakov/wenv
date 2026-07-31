#include "Palette.h"

namespace Wenv::Display
{
Palette::Entry::Entry (int fg, int bg):
	foreground_color {fg},
	background_color {bg}
{		
}

Palette::Palette ()
{
}

Palette::Entry Palette::get_entry (size_t n)
{
	if (n >= colors.size ())
	{
		n = 0;
	}

	if (colors.size () == 0)
	{
		return {};
	}

	return colors[n];
}

Palette::Entry Palette::get_entry (const std::string &n)
{
	size_t nn = 0;
	if (named_colors.find (n) != named_colors.end ())
	{
		nn = named_colors[n];
	}

	return get_entry (nn);
}

} // namespace Wenv::Display


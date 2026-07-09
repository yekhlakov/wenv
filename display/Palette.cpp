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
	colors.push_back ({}); // at least one palette entry must be set
}

Palette::Entry Palette::get_entry (size_t n)
{
	if (n >= colors.size ())
	{
		n = 0;
	}

	return colors[n];
}

} // namespace Wenv::Display


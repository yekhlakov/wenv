#include "Display.h"
#include "../maxy/strings.h"

namespace Wenv::Display
{

// Print character to specified position
void Display::print_char (size_t ln, size_t pos, wchar_t ch)
{
	if (ln >= data.size ()) return;

	if (pos >= data[ln].size ()) return;

	data[ln][pos].set (ch, current_palette_color, current_foreground_color, current_background_color);
}

// Print line left to right starting from specified position
void Display::print_line (size_t ln, size_t pos, const std::wstring & s)
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
void Display::print_line_v (size_t ln, size_t pos, const std::wstring & s)
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


// Draw rectangular box with constant border
void Display::draw_box (size_t ln, size_t pos, size_t w, size_t h, int btype)
{
	if (w < 3 || h < 3)
	{
		return;
	}

	std::wstring elements[] = { 
		maxy::strings::utf8towchar ("═"), 
		maxy::strings::utf8towchar ("║"),
		maxy::strings::utf8towchar ("╔"), 
		maxy::strings::utf8towchar ("╗"), 
		maxy::strings::utf8towchar ("╚"), 
		maxy::strings::utf8towchar ("╝")
	};
//		btype == 2 ? {L'═', L'║', L'╔', L'╗', L'╚', L'╝'} :	// double border
//		btype == 1 ? L"─│┌┐└┘" :	// single border
//		             L"      "; // no border

	std::wstring top = elements[2] + std::wstring (w - 2, elements[0][0]) + elements[3];
	std::wstring bottom = elements[4] + std::wstring (w - 2, elements[0][0]) +elements[5];
	std::wstring vert (h - 2, elements[1][0]);

	print_line (ln, pos, top);
	print_line_v (ln + 1, pos, vert);
	print_line_v (ln + 1, pos + w - 1, vert);
	print_line (ln + h - 1, pos, bottom);
}
	

}

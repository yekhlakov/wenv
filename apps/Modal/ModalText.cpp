#include <vector>
#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../../Context.h"
#include "ModalText.h"

namespace Wenv::Apps
{

void ModalText::draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area)
{
	App::draw (display, path, client_area);

	if (current_context == nullptr)
	{
		return;
	}

	auto text = current_context->get<std::wstring> ("modal-text");
	auto color = current_context->get<std::string> ("modal-text-color");

	auto clr = color ? *color : std::string { ::Wenv::Display::Palette::Default_color };

	// Fill the client area with the highlight background
	current_display->with_color (clr, true);
	current_display->print_line (client_area, L"", current_display->PF_ERASE_BACKGROUND);

	if (text == nullptr || text->empty () || client_area.width < 1 || client_area.height < 1)
	{
		return;
	}

	// Wrap the text to fit the client area
	std::vector<std::wstring> wrapped_lines;
	current_display->get_min_rectangle (*text, 1, 1, client_area.width, client_area.height, &wrapped_lines);

	// Draw the wrapped lines top-aligned in the client area
	current_display->with_color (clr, true);
	for (int i = 0; i < (int) wrapped_lines.size () && i < client_area.height; i++)
	{
		current_display->print_line (client_area.x, client_area.y + i, wrapped_lines[i]);
	}
}

} // namespace Wenv::Apps

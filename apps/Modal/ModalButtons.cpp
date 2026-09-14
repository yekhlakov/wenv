#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../../display/Modal.h"
#include "../../Context.h"
#include "ModalButtons.h"

namespace Wenv::Apps
{

void ModalButtons::draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area)
{
	App::draw (display, path, client_area);

	if (current_context == nullptr)
	{
		return;
	}

	auto buttons = current_context->get<std::vector<::Wenv::Display::ModalButton>> ("modal-buttons");
	auto color = current_context->get<std::string> ("modal-border-color");

	auto clr = color ? *color : std::string { ::Wenv::Display::Palette::Active_element_color };

	// Fill the client area with the highlight background
	current_display->with_color (clr, true);
	current_display->print_line (client_area, L"", current_display->PF_ERASE_BACKGROUND);

	if (buttons == nullptr || buttons->empty ())
	{
		return;
	}

	// Format each button as "[ TEXT ]" separated by spaces
	std::wstring buttons_line;
	for (size_t i = 0; i < buttons->size (); i++)
	{
		if (i > 0)
		{
			buttons_line += L' ';
		}
		buttons_line += L"[ " + (*buttons)[i].text + L" ]";
	}

	current_display->with_color (clr, true);
	current_display->print_line (client_area, buttons_line, current_display->PF_CENTER);
}

} // namespace Wenv::Apps

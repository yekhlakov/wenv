#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../../Context.h"
#include "ModalTitle.h"

namespace Wenv::Apps
{

void ModalTitle::draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area)
{
	App::draw (display, path, client_area);

	if (current_context == nullptr)
	{
		return;
	}

	auto title = current_context->get<std::wstring> ("modal-title");
	auto color = current_context->get<std::string> ("modal-title-color");

	auto clr = color ? *color : std::string { ::Wenv::Display::Palette::Default_color };

	// Fill the client area with the highlight background
	current_display->with_color (clr, true);
	current_display->print_line (client_area, L"", current_display->PF_ERASE_BACKGROUND);

	// Draw the title centered
	if (title != nullptr && !title->empty ())
	{
		current_display->with_color (clr, true);
		current_display->print_line (client_area, *title, current_display->PF_CENTER);
	}
}

} // namespace Wenv::Apps

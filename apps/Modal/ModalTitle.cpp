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

	auto title = get_title ();
	auto clr = *get_title_color ();

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

std::wstring * ModalTitle::get_title ()
{
	// The title is provided by the module that shows the modal; a missing
	// value simply leaves the title block empty
	return current_context->get<std::wstring> ("modal-title");
}

std::string * ModalTitle::get_title_color ()
{
	// Default: the normal text color, also used for the highlighted fill
	return current_context->get<std::string> ("modal-title-color", [] () { return new std::string { ::Wenv::Display::Palette::Default_color }; });
}

} // namespace Wenv::Apps

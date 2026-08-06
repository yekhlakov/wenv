#include <Windows.h>
#include "../display/Display.h"
#include "Context.h"
#include "FuncMenu.h"

namespace Wenv::Apps
{

void FuncMenu::draw (::Wenv::Display::Display & display, const std::string &path, ::Wenv::Display::Rect client_area)
{
	display.print_line
	(
		client_area, 
		L"----<==<{   Func Menu   }>==>----", 
		display.PF_CENTER | display.PF_ERASE_BACKGROUND
	);
}

void FuncMenu::click (::Wenv::Display::Rect client_area, int modifiers)
{
}

void FuncMenu::keypress (int key, int modifiers)
{
	// handle global commands

	// then propagate to focused app
	auto focused_app = current_display->focused_context->get<::Wenv::Apps::App> ("focused-app");
	if (focused_app != nullptr)
	{
		focused_app->with_context(current_display->focused_context)->keypress (key, modifiers);
	}
}

}
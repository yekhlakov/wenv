#include <Windows.h>
#include "../display/Display.h"
#include "FileList.h"
#include "FuncMenu.h"
#include "Context.h"

namespace Wenv::Apps
{

void FuncMenu::draw (::Wenv::Display::Display & display, ::Wenv::Display::Rect client_area)
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
	//current_display->focused_app->keypress (key, modifiers);
}

}
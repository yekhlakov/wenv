#include <Windows.h>
#include "../display/Display.h"
#include "FuncMenu.h"

namespace Wenv::Apps
{

void FuncMenu::draw (::Wenv::Display::Display & display, ::Wenv::Display::Rect client_area)
{
	auto s = name.size ();
	auto x = client_area.x + (client_area.width - s) / 2;
	auto y = client_area.y + (client_area.height) / 2;

	display.print_line (x, y, name);
}

void FuncMenu::click (::Wenv::Display::Rect client_area)
{
}

void FuncMenu::keypress (int key, int modifiers)
{
}

}
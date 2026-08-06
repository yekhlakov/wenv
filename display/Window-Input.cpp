#include <format>
#include "Window.h"
#include "Display.h"
#include "../apps/App.h"
#include "../apps/Context.h"

namespace Wenv::Display
{

void Window::handle_keydown (WPARAM wParam, LPARAM lParam)
{
	key_state[wParam] = true;

	auto focused_app = current_display->focused_context->get<::Wenv::Apps::App> ("focused-app");

	if (focused_app != nullptr)
	{
		int mods = 0;

		if (key_state[VK_CONTROL])
		{
			mods |= 1;
		}
		if (key_state[VK_SHIFT])
		{
			mods |= 2;
		}
		if (key_state[VK_MENU])
		{
			mods |= 4;
		}
		focused_app->with_context (current_display->focused_context)->keypress (wParam, mods);
	}
	draw (hdc);
}

void Window::handle_keyup (WPARAM wParam, LPARAM lParam)
{
	key_state[wParam] = false;
	draw (hdc);
}

void Window::handle_mousemove (WPARAM wParam, LPARAM lParam)
{
	mouse_x = LOWORD (lParam);
	mouse_y = HIWORD (lParam);
	draw (hdc);
}

}

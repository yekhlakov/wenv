#include <format>
#include "Window.h"
#include "Display.h"
#include "../apps/App.h"

namespace Wenv::Display
{

void Window::handle_keydown (WPARAM wParam, LPARAM lParam)
{
	key_state[wParam] = true;

	if (current_display->focused_app != nullptr)
	{
		current_display->focused_app->keypress (wParam, lParam);
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

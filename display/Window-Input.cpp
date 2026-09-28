#include <format>
#include "Window.h"
#include "Display.h"
#include "../apps/App.h"
#include "../Context.h"

namespace Wenv::Display
{

// The current modifier state: 1 = Ctrl, 2 = Shift, 4 = Alt
int Window::current_mods () const
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

	return mods;
}

void Window::handle_keydown (WPARAM wParam, LPARAM lParam)
{
	key_state[wParam] = true;
	dispatch_key_event (wParam, current_mods (), true);
}

void Window::handle_keyup (WPARAM wParam, LPARAM lParam)
{
	key_state[wParam] = false;
	dispatch_key_event (wParam, current_mods (), false);
}

// Dispatch a key press or key release to the apps of the display: while a modal
// is visible every event goes to the modal apps only, otherwise the event is
// forwarded to the apps that want all keypresses (e.g. the func menu) and then
// to the focused app (if any). Escape pops the display, but only when pressed
// (not on release)
void Window::dispatch_key_event (WPARAM wParam, int mods, bool pressed)
{
	if (current_display->current_modal != nullptr)
	{
		// While a modal is visible every key event goes to the modal apps
		// only; the apps of the underlying layouts receive none of them
		auto ctx = current_display->get_context ("modal");

		if (ctx != nullptr)
		{
			forward_key_to_apps (current_display->current_modal->apps, ctx, wParam, mods, pressed);
		}
		invalidate_modified ();
		return;
	}

	if (pressed && wParam == VK_ESCAPE && display_stack.size () > 1)
	{
		pop_display ();

		// The restored display has been fully regenerated - repaint it
		invalidate_modified ();

		return;
	}

	auto focused_context = current_display->focused_context;

	if (focused_context == nullptr)
	{
		// No context has focus - ignore the key event
		return;
	}

	// The event goes to the apps that want every keypress first; once one of
	// them consumes it, the focused app gets nothing
	if (forward_key_to_apps (current_display->listening_apps, focused_context, wParam, mods, pressed))
	{
		invalidate_modified ();
		return;
	}

	auto focused_app = focused_context->get<::Wenv::Apps::App> ("focused-app");

	if (focused_app != nullptr)
	{
		forward_key_to_apps ({ focused_app }, focused_context, wParam, mods, pressed);
	}
	invalidate_modified ();
}

// Call the keydown or keyup handler of each app, bound to the given context,
// until an app consumes the event. Returns whether the event was consumed
bool Window::forward_key_to_apps (
	const std::vector<::Wenv::Apps::App *> &apps,
	::Wenv::Context *ctx,
	WPARAM wParam,
	int mods,
	bool pressed)
{
	for (auto app : apps)
	{
		app->with_context (ctx);

		bool consumed = pressed ? app->handle_keydown (wParam, mods)
		                         : app->handle_keyup (wParam, mods);

		if (consumed)
		{
			return true;
		}
	}

	return false;
}

void Window::handle_mousemove (WPARAM wParam, LPARAM lParam)
{
	mouse_x = LOWORD (lParam);
	mouse_y = HIWORD (lParam);
	invalidate_modified ();
}

void Window::handle_mouse_click (WPARAM wParam, LPARAM lParam)
{
	if (current_display == nullptr || char_width <= 0 || char_height <= 0)
	{
		return;
	}

	// Keep the left button pressed-state table in sync with the up event
	key_state[VK_LBUTTON] = true;

	// The click position in pixels converted to character cells
	auto x = LOWORD (lParam) / char_width;
	auto y = HIWORD (lParam) / char_height;

	// Modifier state: 1 = Ctrl, 2 = Shift, 4 = Alt (the mouse messages report
	// only the first two; alt is read from the pressed-key table)
	int mods = 0;

	if (wParam & MK_CONTROL)
	{
		mods |= 1;
	}
	if (wParam & MK_SHIFT)
	{
		mods |= 2;
	}
	if (key_state[VK_MENU])
	{
		mods |= 4;
	}

	if (current_display->handle_mouse_click (x, y, mods))
	{
		invalidate_modified ();
	}
}

bool Window::get_key_state (int key) const
{
	if (key < 0 || key >= 256)
	{
		return false;
	}

	return key_state[key];
}

}

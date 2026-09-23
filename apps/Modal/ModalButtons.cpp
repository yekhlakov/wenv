#include <Windows.h>
#include <algorithm>
#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../../display/Modal.h"
#include "../../display/Window.h"
#include "../../Context.h"
#include "ModalButtons.h"

namespace Wenv::Apps
{

void ModalButtons::draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area)
{
	App::draw (display, path, client_area);

	// The buttons row is redrawn from its own client area when the active
	// button moves, so the area is kept here instead of resolving a path
	own_area = client_area;

	redraw (path);
}

void ModalButtons::redraw (const std::string &path)
{
	if (current_display == nullptr || current_context == nullptr || own_area.width < 1)
	{
		return;
	}

	auto buttons = get_buttons ();
	auto clr = *get_border_color ();
	auto active = get_active_button ();

	// Fill the client area with the highlight background
	current_display->with_color (clr, true);
	current_display->print_line (own_area, L"", current_display->PF_ERASE_BACKGROUND);

	if (buttons->empty ())
	{
		return;
	}

	// Clamp the active index to the visible buttons so a stale index
	// (from a previously shown modal) cannot point past the list
	if (*active < 0 || *active >= (int) buttons->size ())
	{
		*active = (std::max) (0, (std::min) ((int) buttons->size () - 1, *active));
	}

	// Format each button as "[ TEXT ]" and measure the whole row so it can be
	// drawn centered; each button is drawn separately so it can have its own color
	std::vector<std::wstring> labels;
	labels.reserve (buttons->size ());

	auto total_width = 0;
	for (auto &button : *buttons)
	{
		labels.push_back (L"[ " + button.text + L" ]");
		total_width += (int) labels.back ().size ();
	}
	total_width += (int) buttons->size () - 1;

	// The active button is highlighted, the rest are drawn with the default colors
	auto x = own_area.x + (own_area.width - total_width) / 2;
	for (size_t i = 0; i < labels.size (); i++)
	{
		auto is_active = (int) i == *active;

		current_display->with_color
		(
			is_active ? ::Wenv::Display::Palette::Active_element_color : ::Wenv::Display::Palette::Default_color,
			false
		);
		current_display->print_line (x, own_area.y, labels[i]);

		x += (int) labels[i].size () + 1;
	}

	// Restore the default color so the current colors left on the display do
	// not leak into whatever is drawn afterwards
	current_display->with_color (::Wenv::Display::Palette::Default_color);
}

bool ModalButtons::handle_keydown (unsigned int key, int modifiers)
{
	if (current_display == nullptr || current_context == nullptr)
	{
		return false;
	}

	auto buttons = get_buttons ();
	if (buttons->empty ())
	{
		return false;
	}

	auto active = get_active_button ();

	if (key == VK_RIGHT || key == VK_TAB)
	{
		// Activate the next button, cycling around the list
		*active = (*active + 1) % (int) buttons->size ();
		redraw ("");
		return true;
	}
	else if (key == VK_LEFT)
	{
		// Activate the previous button, cycling around the list
		*active = (*active + (int) buttons->size () - 1) % (int) buttons->size ();
		redraw ("");
		return true;
	}
	else if (key == VK_RETURN)
	{
		// The active button closes the modal, runs its command (if any)
		// and the display is redrawn without the modal on top
		activate (*active);
		return true;
	}

	return false;
}

// Activate the button with the given index as if enter was pressed while it
// was active: its command is run, the modal is closed and the display redrawn
void ModalButtons::activate (int index)
{
	if (current_display == nullptr || current_context == nullptr)
	{
		return;
	}

	auto buttons = get_buttons ();
	if (index < 0 || index >= (int) buttons->size ())
	{
		return;
	}

	// Make the activated button the active one so a later redraw highlights it
	auto active = get_active_button ();
	*active = index;

	auto command = (*buttons)[index].command;

	// Close the modal before running the command so the command observes the
	// display without the modal on top
	current_display->current_modal = nullptr;

	if (command)
	{
		command (current_display);
	}

	auto window = current_display->window;
	if (window != nullptr && window->container_width > 0 && window->container_height > 0)
	{
		// Re-bake the (possibly changed) display so the modal disappears
		window->current_display->resize ((size_t) window->container_width, (size_t) window->container_height);
	}
}

bool ModalButtons::handle_click (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers)
{
	if (current_display == nullptr || current_context == nullptr || client_area.width < 1)
	{
		return false;
	}

	auto buttons = get_buttons ();
	if (buttons->empty ())
	{
		return false;
	}

	// The buttons are drawn centered on the single row of the client area, each
	// as "[ TEXT ]" (width + 4) followed by a one-character gap; recompute those
	// spans so the click can be matched against them
	auto total_width = 0;
	for (auto &button : *buttons)
	{
		total_width += (int) button.text.length () + 4;
	}
	total_width += (int) buttons->size () - 1;

	auto x = (client_area.width - total_width) / 2;
	for (size_t i = 0; i < buttons->size (); i++)
	{
		auto width = (int) (*buttons)[i].text.length () + 4;

		if (position.x >= x && position.x < x + width)
		{
			// A hit on any button activates it, even when it was not active
			activate ((int) i);
			return true;
		}

		x += width + 1;
	}

	return false;
}

std::vector<::Wenv::Display::ModalButton> * ModalButtons::get_buttons ()
{
	// Default: an empty button row, so the app has nothing to draw
	return current_context->get<std::vector<::Wenv::Display::ModalButton>> ("modal-buttons", [] () { return new std::vector<::Wenv::Display::ModalButton> {}; });
}

std::string * ModalButtons::get_border_color ()
{
	// Default: the active color, used for the highlighted border and bar
	return current_context->get<std::string> ("modal-border-color", [] () { return new std::string { ::Wenv::Display::Palette::Active_element_color }; });
}

int * ModalButtons::get_active_button ()
{
	// Default: the first button
	return current_context->get<int> ("modal-active-button", [] () { return new int { 0 }; });
}

} // namespace Wenv::Apps
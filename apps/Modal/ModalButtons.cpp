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

	auto buttons = current_context->get<std::vector<::Wenv::Display::ModalButton>> ("modal-buttons");
	auto color = current_context->get<std::string> ("modal-border-color");
	auto active = current_context->get<int> ("modal-active-button", [] () { return new int { 0 }; });

	auto clr = color ? *color : std::string { ::Wenv::Display::Palette::Active_element_color };

	// Fill the client area with the highlight background
	current_display->with_color (clr, true);
	current_display->print_line (own_area, L"", current_display->PF_ERASE_BACKGROUND);

	if (buttons == nullptr || buttons->empty ())
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
			is_active
		);
		current_display->print_line (x, own_area.y, labels[i]);

		x += (int) labels[i].size () + 1;
	}

	// Restore the default color so the current colors left on the display do
	// not leak into whatever is drawn afterwards
	current_display->with_color (::Wenv::Display::Palette::Default_color);
}

void ModalButtons::keypress (unsigned int key, int modifiers)
{
	if (current_display == nullptr || current_context == nullptr)
	{
		return;
	}

	auto buttons = current_context->get<std::vector<::Wenv::Display::ModalButton>> ("modal-buttons");
	if (buttons == nullptr || buttons->empty ())
	{
		return;
	}

	auto active = current_context->get<int> ("modal-active-button", [] () { return new int { 0 }; });

	if (key == VK_RIGHT || key == VK_TAB)
	{
		// Activate the next button, cycling around the list
		*active = (*active + 1) % (int) buttons->size ();
		redraw ("");
	}
	else if (key == VK_LEFT)
	{
		// Activate the previous button, cycling around the list
		*active = (*active + (int) buttons->size () - 1) % (int) buttons->size ();
		redraw ("");
	}
	else if (key == VK_RETURN)
	{
		// The active button closes the modal, runs its command (if any)
		// and the display is redrawn without the modal on top
		auto command = (*buttons)[*active].command;

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
}

} // namespace Wenv::Apps
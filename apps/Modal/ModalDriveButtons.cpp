#include <Windows.h>
#include "../../display/Modal.h"
#include "../../display/Display.h"
#include "../../Context.h"
#include "ModalDriveButtons.h"

namespace Wenv::Apps
{

std::vector<::Wenv::Display::ModalButton> * ModalDriveButtons::get_buttons ()
{
	// Default: an empty button row, so the app has nothing to draw
	return current_context->get<std::vector<::Wenv::Display::ModalButton>> ("modal-extra-buttons", [] () { return new std::vector<::Wenv::Display::ModalButton> {}; });
}

int * ModalDriveButtons::get_active_button ()
{
	// Default: the first button
	return current_context->get<int> ("modal-extra-active-button", [] () { return new int { 0 }; });
}

// The tab moves the keyboard focus cyclically over all the buttons of the
// modal: the buttons of this row first, then the buttons of the bottom row.
// The row losing the focus drops its highlight (its active index becomes
// negative, which its redraw shows as no active button). The enter activates
// the focused button of this row; when the focus sits in the bottom row, the
// key is left to it. Every other key belongs to the text input above
bool ModalDriveButtons::handle_keydown (unsigned int key, int modifiers)
{
	if (current_display == nullptr || current_context == nullptr)
	{
		return false;
	}

	if (key != VK_TAB && key != VK_RETURN)
	{
		return false;
	}

	auto buttons = get_buttons ();
	auto active = get_active_button ();
	auto row_buttons = current_context->get<std::vector<::Wenv::Display::ModalButton>> ("modal-buttons");
	auto bottom_active = current_context->get<int> ("modal-active-button", [] () { return new int { 0 }; });

	if (buttons->empty () || bottom_active == nullptr)
	{
		return false;
	}

	if (key == VK_RETURN)
	{
		// The bottom row holds the focus, so the key belongs to it
		if (*active < 0)
		{
			return false;
		}

		activate (*active);

		return true;
	}

	// The cycle position runs over this row first, then over the bottom one;
	// it is taken from whichever row currently holds the focus
	auto total = (int) buttons->size () + (row_buttons != nullptr ? (int) row_buttons->size () : 0);
	auto position = *active >= 0 ? *active : (int) buttons->size () + *bottom_active;
	auto next = (position + 1 + total) % total;

	if (next < (int) buttons->size ())
	{
		*active = next;
		*bottom_active = -1;
	}
	else
	{
		*active = -1;
		*bottom_active = next - (int) buttons->size ();
	}

	// Both rows of the modal must reflect the moved focus
	if (current_display->current_modal != nullptr)
	{
		for (auto app : current_display->current_modal->apps)
		{
			app->redraw ("");
		}
	}

	return true;
}

} // namespace Wenv::Apps

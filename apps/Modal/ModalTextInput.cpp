#include <iterator>
#include <vector>
#include <Windows.h>
#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../../Context.h"
#include "ModalTextInput.h"

namespace Wenv::Apps
{

// Convert the keypress into character(s) honoring the shift state and insert
// them into the text just before the cursor; the cursor moves past the
// inserted characters. Keys with ctrl or alt held are shortcuts, not text
static bool insert_typed_chars (unsigned int key, int modifiers, std::wstring &text, int &cursor)
{
	// Keys with ctrl or alt held are shortcuts, not text input
	constexpr int ctrl = 1;
	constexpr int alt = 4;

	if (modifiers & ctrl || modifiers & alt)
	{
		return false;
	}

	// The key to character conversion uses the shift state of the modifiers
	BYTE keys[256] = {};
	if (modifiers & 2)
	{
		keys[VK_SHIFT] = 0x80;
	}

	wchar_t chars[8] = {};
	auto converted = ToUnicode (key, 0, keys, chars, (int) std::size (chars), 0);

	// Zero characters for keys that produce no text, negative for dead keys
	if (converted <= 0)
	{
		return false;
	}

	text.insert (cursor, chars, (size_t) converted);
	cursor += converted;

	return true;
}

void ModalTextInput::draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area)
{
	App::draw (display, path, client_area);

	// The box is redrawn from its own client area when the text changes,
	// so the area is kept here instead of resolving a path
	own_area = client_area;

	redraw (path);
}

void ModalTextInput::redraw (const std::string &path)
{
	if (current_display == nullptr || current_context == nullptr || own_area.width < 1 || own_area.height < 1)
	{
		return;
	}

	auto input = get_input ();
	auto cursor = get_cursor ();
	auto text_clr = *get_text_color ();
	auto border_clr = *get_border_color ();

	// Fill the client area with the highlight background
	current_display->with_color (text_clr, true);
	current_display->print_line (own_area, L"", current_display->PF_ERASE_BACKGROUND);

	// The text block occupies the rows of the block above the input box line
	::Wenv::Display::Rect text_rect = own_area;
	text_rect.height = own_area.height - 1;

	if (text_rect.height >= 1)
	{
		auto text = current_context->get<std::wstring> ("modal-text");

		if (text != nullptr && !text->empty ())
		{
			// Wrap the text to fit the rows above the input box
			std::vector<std::wstring> wrapped_lines;
			current_display->get_min_rectangle (*text, 1, 1, text_rect.width, text_rect.height, &wrapped_lines);

			current_display->with_color (text_clr, true);
			for (int i = 0; i < (int) wrapped_lines.size () && i < text_rect.height; i++)
			{
				current_display->print_line (text_rect.x, text_rect.y + i, wrapped_lines[i]);
			}
		}
	}

	// The input box occupies the last line of the block, its whole width
	::Wenv::Display::Rect box_rect = own_area;
	box_rect.y = own_area.y + own_area.height - 1;
	box_rect.height = 1;

	// The cursor is kept within the text limits
	auto &text_value = *input;
	*cursor = (std::max) (0, (std::min) ((int) text_value.size (), *cursor));

	// The box is drawn as "[ text... ]" framed with spaces; the visible slice
	// of the text is shifted so the cursor always stays inside it
	auto inner_width = box_rect.width - 4;

	if (inner_width >= 1)
	{
		auto visible_begin = *cursor >= inner_width ? *cursor - inner_width + 1 : 0;
		auto visible = text_value.substr (visible_begin, (std::min) (inner_width, (int) text_value.size () - visible_begin));

		current_display->with_color (border_clr, true);
		current_display->print_line (box_rect, L"", current_display->PF_ERASE_BACKGROUND);

		current_display->with_color (::Wenv::Display::Palette::Default_color);
		current_display->print_line
		(
			box_rect.x,
			box_rect.y,
			L"[ " + visible + std::wstring (inner_width - (int) visible.size (), L' ') + L" ]"
		);

		// The text cursor stands in the box at its position within the text
		current_display->cursor_position = { box_rect.x + 2 + *cursor - visible_begin, box_rect.y };
		current_display->is_cursor_visible = true;
	}
}

bool ModalTextInput::handle_keydown (unsigned int key, int modifiers)
{
	if (current_display == nullptr || current_context == nullptr)
	{
		return false;
	}

	auto input = get_input ();
	auto cursor = get_cursor ();

	// The cursor is kept within the text limits
	*cursor = (std::max) (0, (std::min) ((int) input->size (), *cursor));

	if (key == VK_LEFT)
	{
		// The cursor cannot move before the beginning of the text
		*cursor = (std::max) (0, *cursor - 1);
		redraw ("");
		return true;
	}
	else if (key == VK_RIGHT)
	{
		// The cursor cannot move past the end of the text
		*cursor = (std::min) ((int) input->size (), *cursor + 1);
		redraw ("");
		return true;
	}
	else if (key == VK_BACK)
	{
		// Backspace removes the character just before the cursor
		if (*cursor > 0)
		{
			input->erase (*cursor - 1, 1);
			(*cursor)--;
			redraw ("");
		}

		return true;
	}
	else if (key == VK_DELETE)
	{
		// Delete removes the character under the cursor
		if (*cursor < (int) input->size ())
		{
			input->erase (*cursor, 1);
			redraw ("");
		}

		return true;
	}
	else if (key == VK_RETURN || key == VK_TAB)
	{
		// The enter and the tab belong to the buttons app: the enter
		// activates the active button, the tab cycles the buttons
		return false;
	}

	// A regular keypress inserts its character into the text at the cursor;
	// the keys that produce no text fall through to the buttons app
	auto inserted = insert_typed_chars (key, modifiers, *input, *cursor);

	if (inserted)
	{
		redraw ("");
	}

	return inserted;
}

std::wstring * ModalTextInput::get_input ()
{
	// Default: an empty input, so the box starts empty when no prefill is
	// provided by the module that shows the modal
	return current_context->get<std::wstring> ("modal-text-input", [] () { return new std::wstring {}; });
}

int * ModalTextInput::get_cursor ()
{
	// Default: the cursor stands at the beginning of the text
	return current_context->get<int> ("modal-text-input-cursor", [] () { return new int { 0 }; });
}

std::string * ModalTextInput::get_border_color ()
{
	// Default: the active color, used for the highlighted fill of the box
	return current_context->get<std::string> ("modal-border-color", [] () { return new std::string { ::Wenv::Display::Palette::Active_element_color }; });
}

std::string * ModalTextInput::get_text_color ()
{
	// Default: the normal text color, also used for the highlighted fill
	return current_context->get<std::string> ("modal-text-color", [] () { return new std::string { ::Wenv::Display::Palette::Default_color }; });
}

} // namespace Wenv::Apps

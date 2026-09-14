#include <algorithm>
#include <utility>
#include <vector>
#include "Modal.h"
#include "Display.h"
#include "../layout/Grid.h"

namespace Wenv::Display
{

Modal::Modal
(
	const std::wstring &title,
	const std::wstring &text,
	const std::string &border_color,
	const std::string &title_color,
	const std::string &text_color
) :
	title { title },
	text { text },
	border_color { border_color },
	title_color { title_color },
	text_color { text_color },
	grid { new ::Wenv::Layout::Grid {} },
	inner_grid { new ::Wenv::Layout::Grid {} }
{
	// The outer grid is a single cell with a double border hiding the inner grid
	grid->add_row (0, 0, 100);
	grid->add_column (0, 0, 100);
	grid->is_exclusive = true;

	// The inner grid stacks three single-bordered blocks vertically:
	// the title (fixed 3 lines), the text (flexible) and the buttons (fixed 3 lines)
	inner_grid->add_row (3, 3, 0);
	inner_grid->add_row (0, 666, 0);
	inner_grid->add_row (3, 3, 0);
	inner_grid->add_column (0, 0, 100);
	inner_grid->is_exclusive = false;

	inner_grid->add_block ({ 0, 0, 1, 1 }, 1);
	inner_grid->add_block ({ 0, 1, 1, 1 }, 1);
	inner_grid->add_block ({ 0, 2, 1, 1 }, 1);

	grid->add_block ({ 0, 0, 1, 1 }, 2, inner_grid);
}

Modal::~Modal ()
{
	delete grid;
	delete inner_grid;
}

ModalButton *Modal::add_button (const std::wstring &text, ModalButtonAction command)
{
	buttons.push_back ({ text, std::move (command) });

	return &buttons.back ();
}

void Modal::draw (::Wenv::Display::Display &display)
{
	auto display_height = (int) display.data.size ();
	if (display_height < 1)
	{
		return;
	}

	auto display_width = (int) display.data[0].size ();
	if (display_width < 1)
	{
		return;
	}

	// The minimal rectangle that can hold the wrapped text
	std::vector<std::wstring> wrapped_lines;
	auto text_rect = display.get_min_rectangle (text, 1, 1, display_width, display_height, &wrapped_lines);

	// The line that holds all the buttons: each one is rendered as "[ TEXT ]"
	auto buttons_len = 0;
	for (auto &button : buttons)
	{
		buttons_len += (int) button.text.length () + 4;
	}
	if (buttons.size () > 1)
	{
		buttons_len += (int) buttons.size () - 1;
	}

	// The modal must be wide enough for the title, the text and the buttons
	auto content_width = (std::max) ((std::max) ((int) title.length (), text_rect.width), buttons_len);
	auto content_height = (std::max) (text_rect.height, 1);

	// The outer double border adds 2 characters around the inner grid; the stacked
	// single-bordered blocks add 4 more lines for their borders at the outer edges
	auto width = (std::min) (display_width, content_width + 4);
	auto height = (std::min) (display_height, content_height + 8);

	// Center the modal in the display
	auto x = (display_width - width) / 2;
	auto y = (display_height - height) / 2;

	// Bake the modal grid (and its nested inner grid) for the centered rectangle,
	// then draw all its borders on top of the display contents. The borders are
	// drawn in the modal border color (highlight variant)
	std::vector<std::vector<wchar_t>> buffer (display_height, std::vector<wchar_t> (display_width + 1, L' '));
	grid->bake ({ x, y, width, height }, buffer, "modal");

	display.draw_grid (*grid, "modal", nullptr, border_color, true);

	// The three inner blocks: title, text and buttons (paths modal.0.0 .. modal.0.2)
	auto &title_block = inner_grid->blocks[0];
	auto &text_block = inner_grid->blocks[1];
	auto &buttons_block = inner_grid->blocks[2];

	auto title_area = title_block.get_client_dimensions ("modal.0.0");
	auto text_area = text_block.get_client_dimensions ("modal.0.1");
	auto buttons_area = buttons_block.get_client_dimensions ("modal.0.2");

	// Fill the client areas with the highlight background so the underlying display
	// contents (drawn before the modal) do not show through
	display.with_color (title_color, true);
	display.print_line (title_area, L"", display.PF_ERASE_BACKGROUND);
	display.with_color (text_color, true);
	display.print_line (text_area, L"", display.PF_ERASE_BACKGROUND);
	display.with_color (border_color, true);
	display.print_line (buttons_area, L"", display.PF_ERASE_BACKGROUND);

	// Draw the title centered in the top block
	display.with_color (title_color, true);
	display.print_line (title_area, title, display.PF_CENTER);

	// Draw the wrapped text lines, top aligned in the middle block
	display.with_color (text_color, true);
	for (int i = 0; i < (int) wrapped_lines.size () && i < text_area.height; i++)
	{
		display.print_line (text_area.x, text_area.y + i, wrapped_lines[i]);
	}

	// Draw the buttons centered in the bottom block
	std::wstring buttons_line;
	for (size_t i = 0; i < buttons.size (); i++)
	{
		if (i > 0)
		{
			buttons_line += L' ';
		}
		buttons_line += L"[ " + buttons[i].text + L" ]";
	}
	display.with_color (border_color, true);
	display.print_line (buttons_area, buttons_line, display.PF_CENTER);

	// Restore the default color so the current colors left on the display do not
	// leak into whatever is drawn afterwards
	display.with_color (::Wenv::Display::Palette::Default_color);
}

} // namespace Wenv::Display
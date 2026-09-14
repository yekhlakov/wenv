#include <algorithm>
#include <utility>
#include <vector>
#include "Modal.h"
#include "Display.h"
#include "../Context.h"
#include "../layout/Grid.h"

namespace Wenv::Display
{

Modal::Modal
(
	::Wenv::Apps::App *title_app,
	::Wenv::Apps::App *text_app,
	::Wenv::Apps::App *buttons_app,
	const std::string &border_color,
	const std::string &title_color,
	const std::string &text_color
) :
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

	// The three blocks render the modal contents through their attached apps
	inner_grid->add_block ({ 0, 0, 1, 1 }, 1, nullptr, title_app);
	inner_grid->add_block ({ 0, 1, 1, 1 }, 1, nullptr, text_app);
	inner_grid->add_block ({ 0, 2, 1, 1 }, 1, nullptr, buttons_app);

	grid->add_block ({ 0, 0, 1, 1 }, 2, inner_grid);
}

Modal::~Modal ()
{
	delete grid;
	delete inner_grid;
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

	// The content is read from the "modal" context (populated by show_modal);
	// absent values fall back to the modal constructor defaults
	auto ctx = display.get_context ("modal");

	auto title = ctx != nullptr ? ctx->get<std::wstring> ("modal-title") : nullptr;
	auto text = ctx != nullptr ? ctx->get<std::wstring> ("modal-text") : nullptr;
	auto buttons = ctx != nullptr ? ctx->get<std::vector<ModalButton>> ("modal-buttons") : nullptr;
	auto border = ctx != nullptr ? ctx->get<std::string> ("modal-border-color") : nullptr;
	auto title_clr = ctx != nullptr ? ctx->get<std::string> ("modal-title-color") : nullptr;
	auto text_clr = ctx != nullptr ? ctx->get<std::string> ("modal-text-color") : nullptr;

	auto title_text = title ? *title : std::wstring {};
	auto body_text = text ? *text : std::wstring {};
	auto button_list = buttons ? *buttons : std::vector<ModalButton> {};
	auto &border_color_used = border ? *border : border_color;
	auto &title_color_used = title_clr ? *title_clr : title_color;
	auto &text_color_used = text_clr ? *text_clr : text_color;

	// The minimal rectangle that can hold the wrapped text
	std::vector<std::wstring> wrapped_lines;
	auto text_rect = display.get_min_rectangle (body_text, 1, 1, display_width, display_height, &wrapped_lines);

	// The line that holds all the buttons: each one is rendered as "[ TEXT ]"
	auto buttons_len = 0;
	for (auto &button : button_list)
	{
		buttons_len += (int) button.text.length () + 4;
	}
	if (button_list.size () > 1)
	{
		buttons_len += (int) button_list.size () - 1;
	}

	// The modal must be wide enough for the title, the text and the buttons
	auto content_width = (std::max) ((std::max) ((int) title_text.length (), text_rect.width), buttons_len);
	auto content_height = (std::max) (text_rect.height, 1);

	// The outer double border adds 2 characters around the inner grid; the stacked
	// single-bordered blocks add 4 more lines for their borders at the outer edges
	auto width = (std::min) (display_width, content_width + 4);
	auto height = (std::min) (display_height, content_height + 8);

	// Center the modal in the display
	auto x = (display_width - width) / 2;
	auto y = (display_height - height) / 2;

	// Bake the modal grid (and its nested inner grid) for the centered rectangle,
	// then draw all its boundaries on top of the display contents. The attached
	// apps draw the actual contents of the three inner blocks
	std::vector<std::vector<wchar_t>> buffer (display_height, std::vector<wchar_t> (display_width + 1, L' '));
	grid->bake ({ x, y, width, height }, buffer, "modal");

	display.draw_grid (*grid, "modal", ctx, border_color_used, true);

	// Restore the default color so the current colors left on the display do not
	// leak into whatever is drawn afterwards
	display.with_color (::Wenv::Display::Palette::Default_color);
}

} // namespace Wenv::Display
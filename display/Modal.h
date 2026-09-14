#pragma once

#include <functional>
#include <string>
#include <vector>
#include "Palette.h"
#include "../Types.h"

namespace Wenv::Display
{

struct Display;

// The command bound to a modal button
using ModalButtonAction = std::function<void (::Wenv::Display::Display *)>;

// A single modal button: its visible text and the command run when it is activated
struct ModalButton
{
	std::wstring text;
	ModalButtonAction command;
};

// A modal box drawn on top of the display contents.
// A modal stores no content: its title, text and buttons are kept in the "modal"
// context of the display (populated by Display::show_modal), and the contents of
// its three inner blocks are drawn by the ModalTitle, ModalText and ModalButtons
// apps attached to them. The modal itself only positions its grid.
struct Modal
{
	// Fallback palette color names used to draw the border, the title and the text
	// (always drawn in their highlight versions); the values from the "modal"
	// context take precedence when present
	std::string border_color;
	std::string title_color;
	std::string text_color;

	// The grid of this modal: a 1x1 outer grid with a single double-bordered block
	// hiding a 3x1 inner grid of single-bordered blocks (title, text, buttons)
	::Wenv::Layout::Grid *grid = nullptr;

	// The inner grid of the modal (owned along with the outer grid)
	::Wenv::Layout::Grid *inner_grid = nullptr;

	Modal (
		::Wenv::Apps::App *title_app,
		::Wenv::Apps::App *text_app,
		::Wenv::Apps::App *buttons_app,
		const std::string &border_color = ::Wenv::Display::Palette::Active_element_color,
		const std::string &title_color = ::Wenv::Display::Palette::Default_color,
		const std::string &text_color = ::Wenv::Display::Palette::Default_color
	);
	~Modal ();

	// Draw the modal centered on top of the display contents
	void draw (::Wenv::Display::Display &display);
};

} // namespace Wenv::Display
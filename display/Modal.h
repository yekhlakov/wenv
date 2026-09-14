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

// A modal box drawn on top of the display contents
struct Modal
{
	// The title shown in the top block
	std::wstring title;

	// The text shown in the middle block
	std::wstring text;

	// Palette color names used to draw the border, the title and the text
	// (always drawn in their highlight versions)
	std::string border_color;
	std::string title_color;
	std::string text_color;

	// The buttons shown in the bottom block
	std::vector<ModalButton> buttons;

	// The button considered active (nullptr = none)
	ModalButton *active_button = nullptr;

	// The grid of this modal: a 1x1 outer grid with a single double-bordered block
	// hiding a 3x1 inner grid of single-bordered blocks (title, text, buttons)
	::Wenv::Layout::Grid *grid = nullptr;

	// The inner grid of the modal (owned along with the outer grid)
	::Wenv::Layout::Grid *inner_grid = nullptr;

	Modal (
		const std::wstring &title,
		const std::wstring &text,
		const std::string &border_color = ::Wenv::Display::Palette::Active_element_color,
		const std::string &title_color = ::Wenv::Display::Palette::Default_color,
		const std::string &text_color = ::Wenv::Display::Palette::Default_color
	);
	~Modal ();

	// Append a button and return it. The active button must be assigned after all
	// buttons are added (the buttons vector may reallocate on insertion)
	ModalButton *add_button (const std::wstring &text, ModalButtonAction command = {});

	// Draw the modal centered on top of the display contents
	void draw (::Wenv::Display::Display &display);
};

} // namespace Wenv::Display
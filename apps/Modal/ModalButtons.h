#pragma once

#include <string>
#include <vector>
#include "../../display/Modal.h"
#include "../App.h"

namespace Wenv::Apps
{

// The app that draws the button bar in the bottom block of a modal box.
// It reads the button list, the active button index and the border color from
// the "modal" context and draws each button as "[ TEXT ]", centered
// horizontally. The active button is highlighted; the others use the default
// colors. The keys navigate between the buttons (left/right arrows and tab,
// cycling around) and enter activates the active button. A click on a button
// activates that button, even when it is not the currently active one.
class ModalButtons : public App
{
	// The buttons row is redrawn from its own client area when the active
	// button moves, so the area is kept here instead of resolving a path
	::Wenv::Display::Rect own_area;

	// The per-parameter accessors to the current context (see the .cpp)
	std::vector<::Wenv::Display::ModalButton> * get_buttons ();
	std::string * get_border_color ();
	int * get_active_button ();

	// Activate the button with the given index as if enter was pressed while it
	// was active: its command is run, the modal is closed and the display redrawn
	void activate (int index);

public:
	ModalButtons (const std::wstring &n) : App { n } {}

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;

	// Responds to navigation keys and to enter; the core forwards every
	// keypress to this app while its modal is visible
	virtual bool handle_keydown (unsigned int key, int modifiers) override;

	// A click on a button activates it (even when it is not the active one)
	virtual bool handle_click (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers) override;
};

} // namespace Wenv::Apps

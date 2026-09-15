#pragma once

#include <string>
#include "../App.h"

namespace Wenv::Apps
{

// The app that draws the button bar in the bottom block of a modal box.
// It reads the button list, the active button index and the border color from
// the "modal" context and draws each button as "[ TEXT ]", centered
// horizontally. The active button is highlighted; the others use the default
// colors. The keys navigate between the buttons (left/right arrows and tab,
// cycling around) and enter activates the active button.
class ModalButtons : public App
{
	// The buttons row is redrawn from its own client area when the active
	// button moves, so the area is kept here instead of resolving a path
	::Wenv::Display::Rect own_area;

public:
	ModalButtons (const std::wstring &n) : App { n } {}

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;

	// Responds to navigation keys and to enter; the core forwards every
	// keypress to this app while its modal is visible
	virtual void keypress (unsigned int key, int modifiers) override;
};

} // namespace Wenv::Apps

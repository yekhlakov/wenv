#pragma once

#include <string>
#include "../App.h"

namespace Wenv::Apps
{

// The app that draws the body of the middle block of a modal box that asks
// for a line of text: the text block on top (drawn as ModalText does) and
// the text input box below it, occupying the last line of the block and its
// whole width. The typed text and the position of the cursor inside it are
// kept in the "modal" context. The box works as the simplest single-line
// editor: the regular keypresses insert their characters at the cursor, the
// left and right arrows move the cursor (it cannot move beyond the text
// limits), backspace and delete remove a character before/under the cursor
class ModalTextInput : public App
{
	// The box is redrawn from its own client area when the text changes, so
	// the area is kept here instead of resolving a path
	::Wenv::Display::Rect own_area;

	// The per-parameter accessors to the current context (see the .cpp)
	std::wstring * get_input ();
	int * get_cursor ();
	std::string * get_border_color ();
	std::string * get_text_color ();

public:
	ModalTextInput (const std::wstring &n) : App { n } {}

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;

	// Responds to the text editing keys; the core forwards every keypress to
	// this app while its modal is visible, the keys the box does not own
	// (enter, tab) fall through to the buttons app
	virtual bool handle_keydown (unsigned int key, int modifiers) override;
};

} // namespace Wenv::Apps

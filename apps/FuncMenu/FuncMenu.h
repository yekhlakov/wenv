#pragma once
#include <array>
#include <functional>
#include <string>
#include "../App.h"

namespace Wenv::Apps
{

// The action bound to a function key.
// It is called with the menu's display and context.
using FuncMenuAction = std::function<void (::Wenv::Display::Display *, ::Wenv::Context *)>;

// A single function key entry: its visible label and the action to run
struct FuncMenuCommand
{
	std::wstring name;
	FuncMenuAction action;
};

// The func menu bar at the bottom of the screen.
// It shows the labels of the commands attached to the F1..F12 keys.
class FuncMenu : public App
{
	// One slot per function key (F1 .. F12); unbound slots are empty
	std::array<FuncMenuCommand, 12> commands;

	// The func menu is shared between the displays, so it keeps its own client
	// area instead of resolving it from a caller-supplied path
	::Wenv::Display::Rect own_area;

public:
	FuncMenu (const std::wstring &n) : App { n } {}

	// Func menu wants all keypresses
	virtual bool wants_all_keypresses () override { return true; }

	// Bind the given command to a function key (number 1 .. 12)
	void set_command (int number, const std::wstring &name, FuncMenuAction action);

	// Unbind the given function key (number 1 .. 12)
	void erase_command (int number);

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;
	virtual void click (::Wenv::Display::Rect client_area, int modifiers) override;
	virtual void keypress (unsigned int key, int modifiers) override;

	// Redraw all apps sharing the current context
	void redraw_all ();
};

}
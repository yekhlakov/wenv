#pragma once
#include <functional>
#include <string>
#include <vector>
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

// The command list a command is stored in; picked by the pressed modifiers
enum class FuncMenuCommandList
{
	Default,
	Ctrl,
	Alt,
	Shift,
	CtrlShift,
	CtrlAlt
};

// The func menu bar at the bottom of the screen.
// It shows the labels of the commands attached to the F1..F12 keys.
// The commands are stored in the current context under "func_menu.<list>"
// elements; missing lists are empty.
class FuncMenu : public App
{
	// The func menu is shared between the displays, so it keeps its own client
	// area instead of resolving it from a caller-supplied path
	::Wenv::Display::Rect own_area;

	// Fetch the command list for the given modifier combination from the
	// current context. A missing context or a missing element yields an
	// empty list.
	std::vector<FuncMenuCommand> &get_list (FuncMenuCommandList list);

	// The per-parameter accessors to the current context (see the .cpp)
	std::vector<App *> * get_app_group ();
	std::string * get_focused_path ();

public:
	FuncMenu (const std::wstring &n) : App { n } {}

	// Func menu wants all keypresses
	virtual bool wants_all_keypresses () override { return true; }

	// Determine the command list that corresponds to the currently pressed modifiers
	FuncMenuCommandList get_active_command_list () const;

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;
	virtual bool handle_click (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers) override;
	virtual bool handle_keydown (unsigned int key, int modifiers) override;
	virtual bool handle_keyup (unsigned int key, int modifiers) override;

	// Redraw all apps sharing the current context
	void redraw_all ();
};

}
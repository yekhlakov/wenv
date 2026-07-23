#pragma once
#include <string>
#include "../maxy/control/container.h"
#include "../Types.h"

namespace Wenv::Apps
{

// The application
class App
{
protected:
	maxy::control::Container &container;
	std::wstring name;

public:
	App (maxy::control::Container &c, const std::wstring &n) : container { c }, name { n } {}

	// The app may require the core to forward all keypresses to it regardles of current focus
	// (The modal boxes do not allow keypresses propagate to such apps)
	virtual bool wants_all_keypresses () { return false; }

	// The core calls this function when it requires the app to redraw its contents
	virtual void draw (::Wenv::Display::Display & display, ::Wenv::Display::Rect client_area) {};

	// The core calls this function when the app gets focus (from another app)
	virtual void focus () {}

	// The core calls this function when the app loses focus (to another app)
	virtual void blur () {}

	// The core calls this function when the app gets clicked
	virtual void click (::Wenv::Display::Rect client_area) {}

	// The core calls this function when the user presses a key AND the app is in focus
	virtual void keypress (int key, int modifiers) {}
};


}

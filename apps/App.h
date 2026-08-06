#pragma once
#include <string>
#include <unordered_map>
#include "../Types.h"

namespace Wenv::Apps
{

// Application context
// The application must store all its data in this context
// Because several instances of the application on the screen are possible
// and their content may vary.
class Context;


// The application
class App
{
protected:

	std::wstring name;

	::Wenv::Display::Display *current_display = nullptr;
	std::unordered_map<std::string, ::Wenv::Display::Rect> client_areas;

public:
	App (const std::wstring &n) : name { n } {}

	Context *current_context = nullptr;

	// The app may require the core to forward all keypresses to it regardles of current focus
	// (The modal boxes do not allow keypresses propagate to such apps)
	virtual bool wants_all_keypresses () { return false; }

	// Set current context for the application
	virtual App *with_context (Context *c) { current_context = c; return this; }

	// The core calls this function when it requires the app to redraw its contents
	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area)
	{
		current_display = &display;
		client_areas[path] = client_area;
	}

	// The function to redraw the app (when it itself decides to do so)
	virtual void redraw (const std::string &path) {}

	// The core calls this function when the app gets focus (from another app)
	virtual void focus () {}

	// The core calls this function when the app loses focus (to another app)
	virtual void blur () {}

	// The core calls this function when the app gets clicked
	virtual void click (::Wenv::Display::Rect client_area, int modifiers) {}

	// The core calls this function when the user presses a key AND the app is in focus
	virtual void keypress (int key, int modifiers) {}

	virtual ::Wenv::Display::Rect get_client_area (const std::string &path);
};


}

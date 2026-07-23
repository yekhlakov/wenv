#pragma once
#include "App.h"

namespace Wenv::Apps
{

class FuncMenu : public App
{
public:
	FuncMenu (maxy::control::Container &c, const std::wstring &n) : App { c,n } {}

	// Func menu wants all keypresses
	virtual bool wants_all_keypresses () override { return true; }

	virtual void draw (::Wenv::Display::Display &display, ::Wenv::Display::Rect client_area) override;
	virtual void click (::Wenv::Display::Rect client_area) override;
	virtual void keypress (int key, int modifiers) override;
};

}

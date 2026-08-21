#pragma once
#include "App.h"

namespace Wenv::Apps
{

class FuncMenu : public App
{
public:
	FuncMenu (const std::wstring &n) : App { n } {}

	// Func menu wants all keypresses
	virtual bool wants_all_keypresses () override { return true; }

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void click (::Wenv::Display::Rect client_area, int modifiers) override;
	virtual void keypress (unsigned int key, int modifiers) override;
};

}

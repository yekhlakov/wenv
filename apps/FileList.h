#pragma once

#pragma once
#include "App.h"

namespace Wenv::Apps
{

class FileList : public App
{
public:
	FileList (const std::wstring &n) : App { n } {}

	// Func menu wants all keypresses
	virtual bool wants_all_keypresses () override { return true; }

	virtual void draw (::Wenv::Display::Display &display, ::Wenv::Display::Rect client_area) override;
	virtual void redraw () override;
	virtual void click (::Wenv::Display::Rect client_area, int modifiers) override;
	virtual void keypress (int key, int modifiers) override;
};

}

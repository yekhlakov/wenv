#pragma once
#include "../App.h"

namespace Wenv::Apps
{

class FileEditor : public App
{
public:
	FileEditor (const std::wstring &n) : App { n } {}

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;
	virtual void click (::Wenv::Display::Rect client_area, int modifiers) override;
	virtual void keypress (int key, int modifiers) override;

	void redraw_all (const std::string &path);
};

}

#pragma once
#include <string>
#include "../App.h"

namespace Wenv::Apps
{

class FileListHeader : public App
{
	// The per-parameter accessor to the current context (see the .cpp)
	std::wstring * get_pwd ();

public:
	FileListHeader (const std::wstring &n) : App { n } {}

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;
};

}

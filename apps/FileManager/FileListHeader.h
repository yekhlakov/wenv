#pragma once
#include "../App.h"

namespace Wenv::Apps
{

class FileListHeader : public App
{
public:
	FileListHeader (const std::wstring &n) : App { n } {}

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;
};

}

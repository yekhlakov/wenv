#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include "../App.h"

namespace Wenv::Apps
{

class FileInfoShort : public App
{
	// The per-parameter accessors to the current context (see the .cpp)
	std::vector<WIN32_FIND_DATAW> * get_sorted_file_list ();
	std::wstring * get_pwd ();

public:
	FileInfoShort (const std::wstring &n) : App { n } {}

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;
};

}

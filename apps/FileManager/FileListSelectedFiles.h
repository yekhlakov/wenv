#pragma once
#include <string>
#include "../App.h"

namespace Wenv::Apps
{

// The strip between the file list columns and the file info panel showing
// the files selected in the panel
class FileListSelectedFiles : public App
{
public:
	FileListSelectedFiles (const std::wstring &n) : App { n } {}

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;
};

}

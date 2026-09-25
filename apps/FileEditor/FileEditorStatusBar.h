#pragma once

#include <string>
#include "../App.h"

namespace Wenv::Apps
{

class File;

class FileEditorStatusBar : public App
{
	// The per-parameter accessors to the current context (see the .cpp)
	std::wstring * get_edit_target ();
	std::wstring * get_edit_pwd ();
	File * get_file ();
	bool * get_is_editing ();

	int * get_file_top_line (const std::wstring &full_path);
	int * get_file_left_column (const std::wstring &full_path);

public:
	FileEditorStatusBar (const std::wstring &n) : App { n } {}

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;
};

}

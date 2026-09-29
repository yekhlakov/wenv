#pragma once
#include <string>
#include <vector>
#include "../App.h"

namespace Wenv::Apps
{

class File;

// Toggle the editing mode flag of the file editor kept in the given context
void toggle_editing_mode (::Wenv::Context *context);

class FileEditor : public App
{
	// The per-parameter accessors to the current context. They keep the
	// application data behind named methods so the defaults are defined in
	// one place (see the .cpp).
	std::wstring * get_edit_target ();
	std::wstring * get_edit_pwd ();
	std::wstring * get_viewed_path ();
	File * get_file ();
	App * get_status_bar ();
	std::string * get_focused_path ();
	std::vector<App *> * get_app_group ();
	bool * get_is_editing ();

	int * get_file_top_line (const std::wstring &full_path);
	int * get_file_left_column (const std::wstring &full_path);
	int * get_file_cursor_line (const std::wstring &full_path);
	int * get_file_cursor_pos (const std::wstring &full_path);

public:
	FileEditor (const std::wstring &n) : App { n } {}

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;
	virtual bool handle_click (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers) override;
	virtual bool handle_keydown (unsigned int key, int modifiers) override;

	void redraw_all (const std::string &path);
};

}

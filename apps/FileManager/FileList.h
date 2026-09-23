#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include "../App.h"

namespace Wenv::Apps
{

// The file entries of a directory scan (a sorted list of WIN32_FIND_DATAW)
using File_list_type = std::vector<WIN32_FIND_DATAW>;

class FileList : public App
{
	// The per-parameter accessors to the current context (see the .cpp)
	std::wstring * get_pwd ();
	int * get_sort_mode ();
	int * get_list_offset ();
	std::string * get_focused_path ();
	std::vector<App *> * get_app_group ();
	File_list_type * get_file_list ();
	File_list_type * get_sorted_file_list ();

public:

	FileList (const std::wstring &n) : App { n } {}

	// The display string for the given sort mode, e.g. "(↑Name)"
	static std::string get_sort_mode_name (int sort_mode);

	// The file list wants all keypresses
	virtual bool wants_all_keypresses () override { return true; }

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;
	virtual bool handle_click (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers) override;
	virtual bool handle_keydown (unsigned int key, int modifiers) override;

	void redraw_all (const std::string &path);

	inline static const int SORT_MODE_DEFAULT = 0;
	inline static const int SORT_MODE_NAME = 0;
	inline static const int SORT_MODE_REVERSE_NAME = 1;
	inline static const int SORT_MODE_SIZE = 2;
	inline static const int SORT_MODE_REVERSE_SIZE = 3;
	inline static const int SORT_MODE_DATE = 4;
	inline static const int SORT_MODE_REVERSE_DATE = 5;
};

}

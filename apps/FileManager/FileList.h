#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include "../App.h"

namespace Wenv::Apps
{

// The file entries of a directory scan (a sorted list of WIN32_FIND_DATAW)
using File_list_type = std::vector<WIN32_FIND_DATAW>;

// The index of the file selected in the panel of the given context; the
// selection is remembered per directory
int *get_selected_file_idx (::Wenv::Context *c, const std::wstring &dirname);

// Show the file selected in the given file list panel in the file editor
// display, either for viewing or for editing. A selected directory is left
// alone, since descending into it is the business of the file list itself
void show_selected_file (::Wenv::Display::Display *display, ::Wenv::Context *c, bool is_editing);

// Show the "Open a file for editing" modal of the active file list panel.
// The text input box of the modal is prefilled with the name of the file
// highlighted in the panel (when it is a regular file); the Open button of
// the modal opens the typed file in the editor, a name that does not exist
// yet is created when the edited content is saved
void show_open_file_modal (::Wenv::Display::Display *display, ::Wenv::Context *c);

// Show the "Copy a file" modal of the active file list panel. The text input
// box of the modal is prefilled with the working directory of the opposite
// panel; the Copy button copies the file highlighted in the panel to the
// typed path: an existing directory receives the file under its own name,
// the missing intermediate directories are created on the way, a highlighted
// directory is copied along with all its contents. A failed copy shows the
// warning modal with the error message, offering to try again or to cancel.
// The modal does not show up when the ".." entry is selected, since it cannot
// be copied
void show_copy_file_modal (::Wenv::Display::Display *display, ::Wenv::Context *c);

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

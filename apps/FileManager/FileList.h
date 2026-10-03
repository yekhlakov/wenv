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

// The names of the files selected in the panel of the given context, as a
// list for the file operation commands. Currently the single file
// highlighted in the panel is returned; an empty list means there is
// nothing to operate on: there is no selection or the ".." parent entry is
// selected, which cannot be copied, moved or deleted
std::vector<std::wstring> get_selected_file_names (::Wenv::Context *c);

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
// panel; the Copy button copies every file of the given list to the typed
// path: an existing directory receives a file under its own name, the
// missing intermediate directories are created on the way, a directory is
// copied along with all its contents. A file whose target already exists
// brings the modal up that shows the info of both the source and the target
// and offers to overwrite the file (or all such files), to skip it (or all
// such) or to cancel; a file that cannot be copied brings the warning modal
// up with the error message, offering to retry it, to skip the failing file
// (or all the failing ones, when the Skip all is pressed) and go on with the
// rest, or to cancel the whole operation. The modal does not show up when
// the list is empty
void show_copy_file_modal (::Wenv::Display::Display *display, ::Wenv::Context *c, const std::vector<std::wstring> &files);

// Show the "Move a file" modal of the active file list panel: it behaves
// exactly as the copy one, but the files of the given list are moved
// (renamed) to the typed path instead of being copied
void show_move_file_modal (::Wenv::Display::Display *display, ::Wenv::Context *c, const std::vector<std::wstring> &files);

// Show the "Create a directory" modal of the active file list panel with an
// empty text input; the Create button creates the directory by the typed
// path (the whole missing branch of it) in the working directory of the
// panel, a failed creation shows the warning modal with the error message,
// offering to try again or to cancel
void show_mkdir_modal (::Wenv::Display::Display *display);

// Show the confirmation modal of the active file list panel, in the warning
// color: it asks whether to delete the given files, naming them (at most
// three by name, the rest are counted; a directory is named as such, and a
// single one warns that its contents are deleted along with it). The Yes
// button deletes the files with the SHFileOperation, moving them into the
// Recycle Bin (directories are deleted along with all their contents), a
// failed deletion shows the warning modal with the error message, offering
// to retry, to skip the failing file (or all the failing ones, when the
// Skip all is pressed) and go on with the rest, or to cancel. The
// modal does not show up when the list is empty
void show_delete_file_modal (::Wenv::Display::Display *display, ::Wenv::Context *c, const std::vector<std::wstring> &files);

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

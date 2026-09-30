#pragma once
#include <string>
#include <vector>
#include "../App.h"

namespace Wenv::Apps
{

class File;

// Toggle the editing mode flag of the file editor kept in the given context
void toggle_editing_mode (::Wenv::Context *context);

// Drop the undo stack of the file editor kept behind the given context
void clear_undo (::Wenv::Context *context);

// The F6 command of the editor: toggle the editing mode, asking what to do
// with the unsaved changes when leaving the editing mode
void editor_toggle_editing (::Wenv::Display::Display *display, ::Wenv::Context *context);

// The F10 command of the editor: close the editor, asking what to do with
// the unsaved changes when there are any
void editor_exit (::Wenv::Display::Display *display, ::Wenv::Context *context);

// The kind of the undo operation: removing a text range (reverting an
// insertion) or inserting text back (reverting a removal)
enum class UndoType { remove, insert };

// A single undo operation: the action that reverts one editing input
struct UndoOperation
{
	UndoType type = UndoType::remove;

	// The remove operation: the range of lines and symbols to remove, from
	// (start_line, start_pos) to (end_line, end_pos); the end position is
	// just past the removed text
	int start_line = 0;
	int start_pos = 0;
	int end_line = 0;
	int end_pos = 0;

	// The insert operation: the place in the file and the text to insert
	// back; the text may span several lines
	int line = 0;
	int pos = 0;
	std::string text;
};

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

	// The unsaved changes flag shared with the status bar: true while the
	// undo stack of the editing session is not empty
	bool * get_pending_changes ();

	// Update the unsaved changes flag according to the undo stack state
	void update_pending_changes ();

	// The full path of the currently edited file; empty when no file is
	// being edited (no target or working directory set)
	std::wstring get_full_path ();

	int * get_file_top_line (const std::wstring &full_path);
	int * get_file_left_column (const std::wstring &full_path);
	int * get_file_cursor_line (const std::wstring &full_path);
	int * get_file_cursor_pos (const std::wstring &full_path);

	// Convert the typed key into its unicode character and insert it into the
	// edited file at the cursor position; returns false when the key produced
	// no text
	bool insert_typed_char (unsigned int key, int modifiers, int *cursor_line, int *cursor_pos);

	// Remove the character just before the cursor, concatenating the line to
	// the previous one when the cursor is at its beginning; returns false
	// when there is nothing to remove
	bool backspace_at_cursor (int *cursor_line, int *cursor_pos);

	// Remove the character at the cursor, concatenating the next line to this
	// one when the cursor is at or past the line end (the trailing newline of
	// the file when the cursor is past the end of file); returns false when
	// there is nothing to remove
	bool delete_at_cursor (int *cursor_line, int *cursor_pos);

	// The undo stack of the current editing session: the operations that
	// revert the inputs made so far, the most recent last
	std::vector<UndoOperation> undo_stack;

	// Apply the last undo operation and drop it; returns false when the
	// stack is empty
	bool undo_last (int *cursor_line, int *cursor_pos);

	// The apply functions of the two undo operation kinds
	void apply_undo_insert (const UndoOperation &op, int *cursor_line, int *cursor_pos);
	void apply_undo_remove (const UndoOperation &op, int *cursor_line, int *cursor_pos);

public:
	FileEditor (const std::wstring &n) : App { n } {}

	// Drop the undo operations of the editing session
	void clear_undo () { undo_stack.clear (); update_pending_changes (); }

	// Discard all the changes of the session by applying the whole undo
	// stack, emptying it in the process
	void discard_changes ();

	// The placeholder of saving the changes: the actual saving is not
	// implemented yet
	void save_changes ();

	// Show the unsaved changes guard modal that proceeds with the given
	// action ("exit" or "view") when the changes are saved or discarded
	void show_guard_modal (const std::string &action);

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;
	virtual bool handle_click (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers) override;
	virtual bool handle_keydown (unsigned int key, int modifiers) override;

	void redraw_all (const std::string &path);
};

}

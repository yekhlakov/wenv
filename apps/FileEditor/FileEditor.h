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

// The F2 command of the editor: save the file keeping the undo stack; does
// nothing outside the editing mode
void editor_save (::Wenv::Display::Display *display, ::Wenv::Context *context);

// The kind of the undo operation: removing a text range (reverting an
// insertion), inserting text back (reverting a removal) or putting the
// original text back in place of the inserted one (reverting a replacement)
enum class UndoType { remove, insert, replace };

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

	// The replace operation: text holds the text inserted in the place and
	// removed the original text it has replaced; neither spans several lines
	std::string removed;
};

// A range of the selected file characters: from (start_line, start_pos)
// inclusive to (end_line, end_pos) exclusive, the same coordinates the
// removal undo operations use; while the range is being made the end may
// precede the beginning, which selects nothing
struct SelectedRange
{
	int start_line = 0;
	int start_pos = 0;
	int end_line = 0;
	int end_pos = 0;
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
	bool * get_overwrite_mode ();

	// The unsaved changes flag shared with the status bar: true while the
	// undo stack size differs from the size the file was saved with
	bool * get_pending_changes ();

	// Update the unsaved changes flag according to the saved state marker
	// and the current undo stack size
	void update_pending_changes ();

	// The full path of the currently edited file; empty when no file is
	// being edited (no target or working directory set)
	std::wstring get_full_path ();

	int * get_file_top_line (const std::wstring &full_path);
	int * get_file_left_column (const std::wstring &full_path);
	int * get_file_cursor_line (const std::wstring &full_path);
	int * get_file_cursor_pos (const std::wstring &full_path);

	// The editing internals are open to the subclasses, which lets the
	// tests drive the editing and the undo directly
protected:

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

	// Remove the whole first selected range, merging the lines it spans into
	// one; the selection is consumed. Returns false when there is nothing
	// selected to remove
	bool delete_selection (int *cursor_line, int *cursor_pos);

	// Replace the first selected range with the characters produced by the
	// typed key; a selection spanning several lines (or a text with a newline
	// in it) is replaced by its deletion followed by the plain insertion.
	// Returns false when there is no selection or the key produces no text
	bool replace_selection (unsigned int key, int modifiers, int *cursor_line, int *cursor_pos);

	// The first selected range oriented from its earlier end to the later
	// one and clamped to the loaded content; false when there is nothing
	// selected to work with
	bool normalize_selection (int *start_line, int *start_pos, int *end_line, int *end_pos);

	// The text of the first selected range: the tail of its first line, the
	// whole lines between the range ends and the head of the last one,
	// joined with the newline characters; empty when there is nothing
	// selected
	std::string selection_text ();

	// The selected text to the windows clipboard; false when there is
	// nothing selected or the clipboard is not available
	bool copy_selection ();

	// Insert the utf-8 text (a clipboard fragment) at the cursor position,
	// which ends up just past the inserted text; the recorded undo operation
	// removes the inserted text again. Returns false when there is nothing
	// to insert
	bool insert_text (const std::string &utf8, int *cursor_line, int *cursor_pos);

	// The clipboard text pasted at the cursor or in place of the selection
	// when there is one; false when the clipboard holds no text
	bool paste_clipboard (int *cursor_line, int *cursor_pos);

	// Move the cursor to the first character of the nearest word to the left,
	// continuing on the previous lines; stops at the beginning of the file
	void move_word_left (int *cursor_line, int *cursor_pos);

	// Move the cursor to the first character of the nearest word to the right,
	// continuing on the next lines; stops at the end of the file
	void move_word_right (int *cursor_line, int *cursor_pos);

	// The undo stack of the current editing session: the operations that
	// revert the inputs made so far, the most recent last
	std::vector<UndoOperation> undo_stack;

	// The undo stack size at the time the file was saved last: the file
	// counts as edited while the current stack size differs from it. An
	// edit made after the stack has been undone below the saved state sets
	// the marker to -1, since that state cannot be reached by undoing
	// anymore; a freshly opened file starts unedited
	int saved_undo_stack_size = 0;

	// Record the undo operation of a single edit, keeping the saved state
	// marker up to date
	void record_undo (UndoOperation op);

	// Apply the last undo operation and drop it; returns false when the
	// stack is empty
	bool undo_last (int *cursor_line, int *cursor_pos);

	// The apply functions of the three undo operation kinds
	void apply_undo_insert (const UndoOperation &op, int *cursor_line, int *cursor_pos);
	void apply_undo_remove (const UndoOperation &op, int *cursor_line, int *cursor_pos);
	void apply_undo_replace (const UndoOperation &op, int *cursor_line, int *cursor_pos);

	// The selected character ranges of the file; the keyboard and the mouse
	// manipulate the first one of the list
	std::vector<SelectedRange> selected_ranges;

	// True while no keyboard selection is in progress: the next
	// shift-assisted cursor movement drops the first range and starts a new
	// one instead of moving the ends of it
	bool selection_finalized = true;

	// True while the mouse drag selection is in progress: the left button
	// went down on a character and has not been released yet
	bool mouse_selecting = false;

	// The character the drag selection has started on: the far end of the
	// selection, the other one follows the mouse
	int drag_anchor_line = 0;
	int drag_anchor_pos = 0;

	// Account for a cursor movement from the old position to the new one
	// made with the shift key down: the ends of the selection in progress
	// follow the cursor, the finished one is replaced by a new range
	void extend_selection (int old_line, int old_pos, int new_line, int new_pos);

	// Finish the keyboard selection: the shift key has been released; an
	// empty first range selects nothing and is dropped
	void finalize_selection ();

	// Begin the mouse drag selection: the character under the given position
	// becomes the anchor end of the selection, its other end follows the
	// subsequent drags; no range is created until the drag moves
	void begin_mouse_drag (int *top, int *left, int tab_width, ::Wenv::Display::Pos position);

	// The common reaction of the viewer and the editor to the plain press
	// of the left mouse button on a character: the finished selection is
	// cleared by it, the empty one is started as the drag selection; a
	// keyboard selection still in progress is left alone. Returns false
	// when the press changes nothing (the shift-assisted one is not
	// answered here at all)
	bool mouse_press_selection (int modifiers, int *top, int *left, int tab_width, ::Wenv::Display::Pos position);

public:
	FileEditor (const std::wstring &n) : App { n } {}

	// Drop the undo operations of the editing session, which also resets
	// the saved state marker: a fresh session starts over an unedited file
	void clear_undo () { undo_stack.clear (); saved_undo_stack_size = 0; update_pending_changes (); }

	// Forget the selection: used when a new file is opened and when the
	// current one is closed
	void clear_selection () { selected_ranges.clear (); selection_finalized = true; mouse_selecting = false; }

	// Discard all the changes of the session by applying the whole undo
	// stack, emptying it in the process
	void discard_changes ();

	// Save the current content of the file to disk; the undo stack is kept
	// so the session continues, and the file counts as edited again as
	// soon as anything is edited or undone
	void save_changes ();

	// Show the unsaved changes guard modal that proceeds with the given
	// action ("exit" or "view") when the changes are saved or discarded
	void show_guard_modal (const std::string &action);

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;
	virtual bool handle_click (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers) override;
	virtual bool handle_keydown (unsigned int key, int modifiers) override;
	virtual bool handle_keyup (unsigned int key, int modifiers) override;
	virtual bool handle_mouse_drag (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers) override;

	void redraw_all (const std::string &path);
};

}

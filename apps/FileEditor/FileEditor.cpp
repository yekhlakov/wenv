#include <Windows.h>
#include <iterator>
#include <string>
#include <vector>
#include "../../display/Display.h"
#include "../../display/Window.h"
#include "../../display/Palette.h"
#include "../../maxy/strings.h"
#include "../../Context.h"
#include "../FuncMenu/FuncMenu.h"
#include "File.h"
#include "FileEditor.h"

namespace Wenv::Apps
{

void toggle_editing_mode (::Wenv::Context *context)
{
	if (context == nullptr)
	{
		return;
	}

	// The flag is stored in the editor context; a file that has just been
	// opened is viewed, not edited
	auto is_editing = context->get<bool> ("is_editing", [] () { return new bool { false }; });

	*is_editing = !*is_editing;

	// Every switch between viewing and editing starts over in the insert mode
	auto overwrite_mode = context->get<bool> ("overwrite-mode", [] () { return new bool { false }; });

	*overwrite_mode = false;
}

void clear_undo (::Wenv::Context *context)
{
	if (context == nullptr)
	{
		return;
	}

	// The focused app is stored as its base type, the editor is recognized
	// by the dynamic type
	auto app = context->get<App> ("focused-app");
	auto editor = dynamic_cast<FileEditor *> (app);

	if (editor != nullptr)
	{
		editor->clear_undo ();
	}
}

// The file editor app of the given display, null when it is absent
static FileEditor * get_editor_app (::Wenv::Display::Display *display)
{
	auto ctx = display != nullptr ? display->get_context ("file-editor") : nullptr;

	return ctx == nullptr ? nullptr : dynamic_cast<FileEditor *> (ctx->get<App> ("focused-app"));
}

// Proceed with the action that requested the guard modal: exit the editor
// or switch it to the viewing mode; the changes are dropped either way
static void guard_proceed (::Wenv::Display::Display *display)
{
	auto ctx = display != nullptr ? display->get_context ("file-editor") : nullptr;

	if (ctx == nullptr)
	{
		return;
	}

	// The changes do not survive the action
	clear_undo (ctx);

	// The action remembered by the guard modal
	auto action = ctx->get<std::string> ("guard-action");

	if (action == nullptr)
	{
		return;
	}

	if (*action == "exit")
	{
		editor_exit (display, ctx);
	}
	else if (*action == "view")
	{
		toggle_editing_mode (ctx);
	}
}

// The "Save the changes" button: the (yet unimplemented) saving is followed
// by the action that requested the guard
static void guard_save (::Wenv::Display::Display *display)
{
	auto editor = get_editor_app (display);

	if (editor != nullptr)
	{
		editor->save_changes ();
	}

	guard_proceed (display);
}

// The "Discard the changes" button: the changes are reverted by applying
// the whole undo stack and the action that requested the guard proceeds
static void guard_discard (::Wenv::Display::Display *display)
{
	auto editor = get_editor_app (display);

	if (editor != nullptr)
	{
		editor->discard_changes ();
	}

	guard_proceed (display);
}

void editor_toggle_editing (::Wenv::Display::Display *display, ::Wenv::Context *context)
{
	if (context == nullptr)
	{
		return;
	}

	auto is_editing = context->get<bool> ("is_editing", [] () { return new bool { false }; });
	auto pending = context->get<bool> ("pending-changes", [] () { return new bool { false }; });

	// Leaving the editing mode with unsaved changes asks what to do with
	// them first
	if (*is_editing && *pending)
	{
		auto editor = get_editor_app (display);

		if (editor != nullptr)
		{
			editor->show_guard_modal ("view");
			return;
		}
	}

	toggle_editing_mode (context);
	clear_undo (context);
}

void editor_exit (::Wenv::Display::Display *display, ::Wenv::Context *context)
{
	auto pending = context != nullptr
		? context->get<bool> ("pending-changes", [] () { return new bool { false }; })
		: nullptr;

	// Closing the editor with unsaved changes asks what to do with them
	// first
	if (pending != nullptr && *pending)
	{
		auto editor = get_editor_app (display);

		if (editor != nullptr)
		{
			editor->show_guard_modal ("exit");
			return;
		}
	}

	// No changes or no editor to ask: the changes are dropped and the
	// editor display is closed
	clear_undo (context);

	// The selection is not kept across the closed files: opening the file
	// again starts with none
	auto editor = get_editor_app (display);

	if (editor != nullptr)
	{
		editor->clear_selection ();
	}

	if (display == nullptr || display->window == nullptr)
	{
		return;
	}

	display->window->close_current_display ();
}

// The F2 command of the editor: save the current content of the file
// without touching the undo stack; only the editing mode saves
void editor_save (::Wenv::Display::Display *display, ::Wenv::Context *context)
{
	if (context == nullptr || !*context->get<bool> ("is_editing", [] () { return new bool { false }; }))
	{
		return;
	}

	auto editor = get_editor_app (display);

	if (editor != nullptr)
	{
		editor->save_changes ();
	}
}

// The lexicographic order of the (line, pos) positions of the file
static bool pos_less (int line_a, int pos_a, int line_b, int pos_b)
{
	return line_a != line_b ? line_a < line_b : pos_a < pos_b;
}

// Overdraw the characters of the given line covered by the selected range
// with the selected color variant of each character color: the text keeps
// its default color, the tab arrows and the trailing space dots keep their
// dark one. The range may point in either direction (the end may precede
// the beginning while the selection is being made), so the covered band is
// taken between its outermost positions. The default color is restored
// before returning
static void draw_selected_band (
	::Wenv::Display::Display *display,
	const ::Wenv::Display::Rect &area,
	int row_y,
	int left,
	File *file,
	int line,
	const std::wstring &expanded,
	const std::vector<std::pair<int, int>> &tab_spans,
	int tab_width,
	int trailing_start,
	const SelectedRange &range)
{
	// The range oriented from the earlier position to the later one
	auto from_line = range.start_line;
	auto from_pos = range.start_pos;
	auto to_line = range.end_line;
	auto to_pos = range.end_pos;

	if (pos_less (to_line, to_pos, from_line, from_pos))
	{
		std::swap (from_line, to_line);
		std::swap (from_pos, to_pos);
	}

	// The line carries no part of the range
	if (line < from_line || line > to_line)
	{
		return;
	}

	// The part of the range falling on this line, in the display columns of
	// the tab-expanded text; a line between the range ends is covered whole
	auto band_from = from_line == line ? file_line_display_pos (file, line, from_pos, tab_width) : 0;
	auto band_to = to_line == line ? file_line_display_pos (file, line, to_pos, tab_width) : (int) expanded.size ();

	// Only the part visible in the viewport is drawn, and only where the
	// line has characters (the selected newlines have nothing to show on)
	auto from = max (band_from, left);
	auto to = min (band_to, left + area.width);
	auto text_to = min (to, (int) expanded.size ());

	if (text_to <= from)
	{
		return;
	}

	// The characters are printed again in the selected variant of their own
	// color; the spaces the expansion of the selected tabs consists of are
	// drawn over as well
	display->with_color (::Wenv::Display::Palette::Default_color, ::Wenv::Display::Palette::Variant_selected);

	::Wenv::Display::Rect band
	{
		area.x + from - left,
		row_y,
		text_to - from,
		1
	};

	display->print_line
	(
		band,
		expanded.substr (from, text_to - from),
		display->PF_TOP | display->PF_LEFT | display->PF_ERASE_BACKGROUND
	);

	// The tab arrows and the trailing space dots within the band keep their
	// dark color (in its selected variant)
	display->with_color (::Wenv::Display::Palette::Dark_element_color, ::Wenv::Display::Palette::Variant_selected);

	for (const auto &[pos, len] : tab_spans)
	{
		if (pos >= from && pos < text_to)
		{
			display->print_char (area.x + pos - left, row_y, L'\u2192');
		}
	}

	if (trailing_start >= 0)
	{
		for (int col = max (from, trailing_start); col < text_to; col++)
		{
			display->print_char (area.x + col - left, row_y, L'\u00B7');
		}
	}

	display->with_color (::Wenv::Display::Palette::Default_color);
}

void FileEditor::draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area)
{
	App::draw (display, path, client_area);

	if (current_context == nullptr)
	{
		return;
	}

	redraw (path);
}

void FileEditor::redraw (const std::string &path)
{
	auto full_path = get_full_path ();

	if (full_path.empty ())
	{
		return;
	}

	auto file = get_file ();
	auto viewed_path = get_viewed_path ();

	if (file == nullptr || viewed_path == nullptr || *viewed_path != full_path)
	{
		// A different file was requested - load it
		file = new File { full_path };

		current_context->set ("file", file);
		current_context->set ("viewed-path", new std::wstring { full_path });

		// The selection is not kept across the opened files, but survives
		// the switches between the viewing and the editing modes
		clear_selection ();
	}

	// The tab width is a persistent setting, shared by the editor and its
	// status bar; the file re-measures the content when it changes
	auto tab_width = *get_tab_width (current_display->get_persistent_context ());

	file->set_tab_width (tab_width);

	auto area = get_client_area (path);

	// Store position in persistent context per file, keyed by full path
	auto top = get_file_top_line (full_path);
	auto left = get_file_left_column (full_path);

	// Load more data if the viewport is near the end of loaded content
	file->ensure_loaded (*top, area.height);

	// Clamp the visible window to the content
	auto line_count = file->get_line_count ();
	if (line_count != UNKNOWN_LINE_COUNT)
	{
		*top = min (*top, max (0, line_count - area.height));
	}
	*top = max (*top, 0);
	*left = min (*left, max (0, (int) file->longest_expanded - area.width));
	*left = max (*left, 0);

	// The editing mode state and the per-file cursor position
	auto is_editing = get_is_editing ();
	auto overwrite_mode = get_overwrite_mode ();
	auto cursor_line = get_file_cursor_line (full_path);
	auto cursor_pos = get_file_cursor_pos (full_path);

	// The func menu commands follow the editing mode: the F2 "Save" command
	// is shown only while editing (a viewed file has nothing to save), and
	// the F6 command offers the mode it would switch to
	auto commands = current_context->get<std::vector<FuncMenuCommand>> ("func_menu.default");

	if (commands != nullptr)
	{
		(*commands)[1].name = *is_editing ? L"Save" : L"";
		(*commands)[5].name = *is_editing ? L"View" : L"Edit";
	}

	// The display position of the cursor within its line. The cursor is kept
	// in the raw (before the tab expansion) coordinates of the file, and a tab
	// under the cursor is displayed at the beginning of its expansion
	auto cursor_display_pos = 0;

	if (*is_editing)
	{
		cursor_display_pos = file_line_display_pos (file, *cursor_line, *cursor_pos, tab_width);

		// The viewport follows the cursor: when the cursor is outside the
		// visible portion, the viewport is shifted so that the cursor stands
		// at the corresponding edge of it
		if (*cursor_line < *top)
		{
			*top = *cursor_line;
		}

		if (*cursor_line >= *top + area.height)
		{
			*top = *cursor_line - area.height + 1;
		}

		if (cursor_display_pos < *left)
		{
			*left = cursor_display_pos;
		}

		if (cursor_display_pos >= *left + area.width)
		{
			*left = cursor_display_pos - area.width + 1;
		}
	}

	// The end-of-file marker is shown when the whole file has been loaded: on
	// the empty line after the content when the file ends with a newline (the
	// loader keeps it as the trailing empty line), past the content of the
	// last line otherwise
	int eof_line = -1;
	int eof_col = 0;

	if (line_count != UNKNOWN_LINE_COUNT)
	{
		if (file->lines.empty ())
		{
			eof_line = 0;
		}
		else
		{
			eof_line = (int) file->lines.size () - 1;

			if (!file->lines.back ().raw_data.empty ())
			{
				eof_col = (int) expand_tabs (file->lines.back ().raw_data, tab_width).first.size ();
			}
		}
	}

	int ln = *top;
	auto it = file->lines.begin ();
	std::advance (it, min (*top, (int) file->lines.size ()));

	// Set the default color
	current_display->with_color (::Wenv::Display::Palette::Default_color);

	for (int row = 0; row < area.height; row++)
	{
		::Wenv::Display::Rect r = area;
		r.y += row;
		r.height = 1;

		std::wstring expanded;
		std::vector<std::pair<int, int>> tab_spans;
		if (ln < (int) file->lines.size ())
		{
			auto result = expand_tabs (it->raw_data, tab_width);
			expanded = std::move (result.first);
			tab_spans = std::move (result.second);
			++it;
		}

		ln++;

		auto s = *left < (int) expanded.size ()
			? expanded.substr ((size_t) *left)
			: std::wstring {};

		bool truncated = (int) s.size () > area.width && area.width > 1;

		if (s.empty ())
		{
			s = L"";
			truncated = false;
		}

		current_display->print_line
		(
			r,
			s,
			current_display->PF_TOP | current_display->PF_LEFT | current_display->PF_ERASE_BACKGROUND
		);

		bool drew_dark = false;

		// The display column the trailing spaces of the line start at; -1
		// when the line has none (the dots are drawn for them)
		int trailing_start = -1;

		// Draw trailing spaces as middle dots
		if (!expanded.empty ())
		{
			auto last_non_space = expanded.find_last_not_of (L' ');

			// An all-whitespace line is all trailing spaces
			trailing_start = last_non_space == std::wstring::npos ? 0 : (int) last_non_space + 1;
			int trailing_count = (int) expanded.size () - trailing_start;

			if (trailing_count > 0)
			{
				current_display->with_color (::Wenv::Display::Palette::Dark_element_color);
				drew_dark = true;

				for (int i = 0; i < trailing_count; i++)
				{
					int screen_x = trailing_start + i - *left;
					if (screen_x >= 0 && screen_x < area.width)
					{
						current_display->print_char (area.x + screen_x, r.y, L'\u00B7');
					}
				}
			}
		}

		// Draw tab markers in a separate pass using stored spans
		if (tab_spans.size() > 0)
		{
			if (!drew_dark)
			{
				current_display->with_color (::Wenv::Display::Palette::Dark_element_color);
				drew_dark = true;
			}
			for (auto &[pos, len] : tab_spans)
			{
				int screen_x = pos - *left;
				int vis_start = max (0, screen_x);
				int vis_end = min (area.width, screen_x + len);
				int vis_width = vis_end - vis_start;

				if (vis_width > 0)
				{
					::Wenv::Display::Rect tab_rect;
					tab_rect.x = area.x + vis_start;
					tab_rect.y = r.y;
					tab_rect.width = vis_width;
					tab_rect.height = 1;

					current_display->print_line
					(
						tab_rect,
						L"\u2192",
						current_display->PF_LEFT | current_display->PF_VCENTER | current_display->PF_ERASE_BACKGROUND
					);
				}
			}
		}

		// The end-of-file marker past the content of the last line
		if (area.width > 0 && *top + row == eof_line)
		{
			auto marker_screen_x = eof_col - *left;

			if (marker_screen_x < area.width)
			{
				auto marker = std::wstring { L"<EOF>" };

				// Only the visible part of the marker is drawn
				if (marker_screen_x < 0)
				{
					marker = marker.substr ((size_t) -marker_screen_x);
					marker_screen_x = 0;
				}

				marker = marker.substr (0, min ((int) marker.size (), area.width - marker_screen_x));

				if (!drew_dark)
				{
					current_display->with_color (::Wenv::Display::Palette::Dark_element_color);
					drew_dark = true;
				}

				current_display->print_line (area.x + marker_screen_x, r.y, marker);
			}
		}

		// Colorize the truncation ellipsis with the active palette color
		if (truncated)
		{
			current_display->with_color (::Wenv::Display::Palette::Active_element_color);
			current_display->print_char (area.x + area.width - 1, r.y, L'\u2026');
		}

		if (drew_dark || truncated)
		{
			// Return the default color if we've changed it to some other color previously
			current_display->with_color (::Wenv::Display::Palette::Default_color);
		}

		// The characters covered by the selected ranges are drawn over with
		// the selected variants of their own colors
		for (const auto &range : selected_ranges)
		{
			draw_selected_band (current_display, area, r.y, *left, file, ln - 1, expanded, tab_spans, tab_width, trailing_start, range);
		}
	}

	// The text cursor is hidden in the viewing mode; in the editing mode it is
	// shown at the position computed from the cursor position in the file and
	// the currently visible portion of it. The overwrite mode highlights the
	// whole character under it
	current_display->is_cursor_visible = *is_editing;
	current_display->is_cursor_overwrite = *is_editing && *overwrite_mode;

	if (*is_editing)
	{
		current_display->cursor_position =
		{
			area.x + cursor_display_pos - *left,
			area.y + *cursor_line - *top
		};
	}

	// Update the status bar
	auto status = get_status_bar ();
	if (status != nullptr)
	{
		status->with_context (current_context)->redraw (path);
	}
}

bool FileEditor::handle_click (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers)
{
	if (current_context == nullptr)
	{
		return false;
	}

	auto full_path = get_full_path ();

	if (full_path.empty ())
	{
		return false;
	}

	// The click lands on the visible portion of the content: the row and
	// the column of the click within it. A click outside the content bounds
	// is fine: the cursor positions itself past the content the same way
	// the keyboard navigation does
	auto top = get_file_top_line (full_path);
	auto left = get_file_left_column (full_path);
	auto tab_width = *get_tab_width (current_display->get_persistent_context ());

	// The viewing mode has no cursor to move: the mouse press is answered
	// by the selection reaction only
	if (!*get_is_editing ())
	{
		auto handled = mouse_press_selection (modifiers, top, left, tab_width, position);

		if (handled)
		{
			redraw_all (*get_focused_path ());
		}

		return handled;
	}

	auto line = *top + position.y;
	auto column = *left + position.x;

	// The plain press makes its common selection changes; the shift one is
	// answered by the extension below. The cursor is moved as usual in the
	// either case
	mouse_press_selection (modifiers, top, left, tab_width, position);

	// The modifier flag of the shift key (see Window::current_mods)
	constexpr int shift = 2;

	auto cursor_line = get_file_cursor_line (full_path);
	auto cursor_pos = get_file_cursor_pos (full_path);

	// The place the cursor moves from: with the shift key down it becomes
	// the second end of the extended selection
	auto old_line = *cursor_line;
	auto old_pos = *cursor_pos;

	*cursor_line = line;
	*cursor_pos = file_line_raw_pos (get_file (), line, column, tab_width);

	if (modifiers & shift)
	{
		extend_selection (old_line, old_pos, *cursor_line, *cursor_pos);
	}

	// The redraw updates the cursor position of the display and marks the
	// characters modified, so the cursor is actually repainted at the new
	// place (it is drawn within the invalidated region only)
	redraw_all (*get_focused_path ());

	return true;
}

bool FileEditor::handle_keydown (unsigned int key, int modifiers)
{
	if (current_context == nullptr)
	{
		return false;
	}

	auto path = *get_focused_path ();
	auto area = get_client_area (path);

	// Reconstruct full path to look up per-file position in persistent context
	auto full_path = get_full_path ();

	if (full_path.empty ())
	{
		return false;
	}

	auto top = get_file_top_line (full_path);
	auto left = get_file_left_column (full_path);
	auto is_editing = get_is_editing ();

	// The modifier flag of the control key (see Window::current_mods)
	constexpr int ctrl = 1;

	// The modifier flag of the shift key (see Window::current_mods)
	constexpr int shift = 2;

	// The Escape key is owned by the editor: it closes the editor unless
	// it is editing a file with unsaved changes, which cannot be lost by
	// an accidental keypress
	if (key == VK_ESCAPE)
	{
		if (!*is_editing || !*get_pending_changes ())
		{
			editor_exit (current_display, current_context);
		}

		return true;
	}

	// The clipboard keys copy the selected text in both modes; the cut in
	// the editing mode also removes the selected fragment, the same way the
	// Delete key does
	if ((modifiers & ctrl) && (key == 'C' || key == 'X') && copy_selection ())
	{
		if (key == 'X' && *is_editing)
		{
			auto cursor_line = get_file_cursor_line (full_path);
			auto cursor_pos = get_file_cursor_pos (full_path);

			delete_selection (cursor_line, cursor_pos);
			update_pending_changes ();
		}

		// The redraw shows the selection consumed by the cut
		redraw_all (path);

		return true;
	}

	if (*is_editing)
	{
		// The editing mode: the keys move the cursor in the raw coordinates
		// of the file, the viewport follows its display position
		auto cursor_line = get_file_cursor_line (full_path);
		auto cursor_pos = get_file_cursor_pos (full_path);

		// The place the cursor moves from: with the shift key down it is
		// one of the ends of the extended selection
		auto old_line = *cursor_line;
		auto old_pos = *cursor_pos;

		// The keys that move the cursor: the shift-assisted ones extend the
		// selection
		bool moves_cursor = key == VK_UP || key == VK_DOWN || key == VK_LEFT || key == VK_RIGHT
			|| key == VK_PRIOR || key == VK_NEXT || key == VK_HOME || key == VK_END;

		// The movement keys always succeed; the text editing keys report
		// whether they actually changed anything
		bool handled = true;

		// The vertical movement keeps the display column of the cursor,
		// which may differ from its logical position on a line with tabs
		bool vertical = key == VK_UP || key == VK_DOWN || key == VK_PRIOR || key == VK_NEXT;
		auto tab_width = *get_tab_width (current_display->get_persistent_context ());
		auto keep_column = vertical
			? file_line_display_pos (get_file (), *cursor_line, *cursor_pos, tab_width)
			: -1;

		if (key == VK_UP)
		{
			(*cursor_line)--;
		}
		else if (key == VK_DOWN)
		{
			(*cursor_line)++;
		}
		else if (key == VK_LEFT)
		{
			// Ctrl+Left moves by words, plain Left moves by characters
			if (modifiers & ctrl)
			{
				move_word_left (cursor_line, cursor_pos);
			}
			else
			{
				(*cursor_pos)--;
			}
		}
		else if (key == VK_RIGHT)
		{
			// Ctrl+Right moves by words, plain Right moves by characters
			if (modifiers & ctrl)
			{
				move_word_right (cursor_line, cursor_pos);
			}
			else
			{
				(*cursor_pos)++;
			}
		}
		else if (key == VK_PRIOR)
		{
			*cursor_line -= area.height > 0 ? area.height : 1;
		}
		else if (key == VK_NEXT)
		{
			*cursor_line += area.height > 0 ? area.height : 1;
		}
		else if (key == VK_HOME)
		{
			*cursor_pos = 0;

			if (modifiers & ctrl)
			{
				*cursor_line = 0;
			}
		}
		else if (key == VK_END)
		{
			if (modifiers & ctrl)
			{
				auto file = get_file ();
				auto line_count = file != nullptr ? file->get_line_count () : UNKNOWN_LINE_COUNT;

				// The last line of a not fully loaded file is unknown - the
				// key is ignored for such files
				if (line_count == UNKNOWN_LINE_COUNT)
				{
					return true;
				}

				*cursor_line = line_count - 1;
				*cursor_pos = file_line_length (get_file (), *cursor_line);
			}
			else
			{
				*cursor_pos = file_line_length (get_file (), *cursor_line);
			}
		}
		else if (key == VK_INSERT)
		{
			// The Insert key switches between inserting and overwriting text
			auto overwrite_mode = get_overwrite_mode ();

			*overwrite_mode = !*overwrite_mode;
		}
		else if (key == VK_BACK)
		{
			// Backspace removes the whole selection when there is one,
			// otherwise the character just before the cursor
			handled = delete_selection (cursor_line, cursor_pos)
				|| backspace_at_cursor (cursor_line, cursor_pos);
		}
		else if (key == VK_DELETE)
		{
			// Delete removes the whole selection when there is one,
			// otherwise the character at the cursor
			handled = delete_selection (cursor_line, cursor_pos)
				|| delete_at_cursor (cursor_line, cursor_pos);
		}
		else if (key == 'Z' && (modifiers & ctrl))
		{
			// Ctrl+Z applies and drops the last undo operation
			handled = undo_last (cursor_line, cursor_pos);
		}
		else if (key == 'V' && (modifiers & ctrl))
		{
			// Ctrl+V pastes the clipboard text: in place of the selection
			// when there is one, at the cursor place otherwise
			handled = paste_clipboard (cursor_line, cursor_pos);
		}
		else
		{
			// A regular keypress: it replaces the selection when there is
			// one, otherwise it is inserted at the cursor position
			handled = replace_selection (key, modifiers, cursor_line, cursor_pos)
				|| insert_typed_char (key, modifiers, cursor_line, cursor_pos);
		}

		// A no-op text editing key is reported as unhandled
		if (!handled)
		{
			return false;
		}

		// The editing keys may have changed the undo stack, which shows up
		// in the unsaved changes flag of the context
		update_pending_changes ();

		// The kept display column is converted back to the logical position
		// in the line the cursor has arrived to
		if (vertical)
		{
			*cursor_pos = file_line_raw_pos (get_file (), *cursor_line, keep_column, tab_width);
		}

		// The cursor cannot go beyond the first line and its first position
		*cursor_line = max (*cursor_line, 0);
		*cursor_pos = max (*cursor_pos, 0);

		// The shift-assisted cursor movement extends the selection; the
		// movement without the shift key leaves the selection alone
		if (moves_cursor && (modifiers & shift))
		{
			extend_selection (old_line, old_pos, *cursor_line, *cursor_pos);
		}
	}
	else
	{
		// The viewing mode: the keys scroll the viewport
		if (key == VK_UP)
		{
			(*top)--;
		}
		else if (key == VK_DOWN)
		{
			(*top)++;
		}
		else if (key == VK_LEFT)
		{
			(*left)--;
		}
		else if (key == VK_RIGHT)
		{
			(*left)++;
		}
		else if (key == VK_PRIOR)
		{
			*top -= area.height > 0 ? area.height : 1;
		}
		else if (key == VK_NEXT)
		{
			*top += area.height > 0 ? area.height : 1;
		}
		else
		{
			return false;
		}
	}

	// The offsets are clamped to the content bounds during redraw
	redraw_all (path);

	return true;
}

bool FileEditor::handle_keyup (unsigned int key, int modifiers)
{
	// The released shift key finishes the keyboard selection: the next
	// shift-assisted movement starts a new range instead of moving the ends
	// of this one
	if (key == VK_SHIFT && !selected_ranges.empty ())
	{
		finalize_selection ();

		// The finalized range may have been dropped
		return true;
	}

	// The released left mouse button finishes the drag selection started by
	// its press, the same way the released shift key finishes the keyboard
	// one
	if (key == VK_LBUTTON && mouse_selecting)
	{
		mouse_selecting = false;
		finalize_selection ();

		// The finalized range may have been dropped
		return true;
	}

	return false;
}

void FileEditor::extend_selection (int old_line, int old_pos, int new_line, int new_pos)
{
	// The selection in progress keeps being made: the end the cursor sits
	// on follows the movement
	bool keeps_making = !selection_finalized && !selected_ranges.empty ();

	if (keeps_making)
	{
		auto &first = selected_ranges.front ();

		if (old_line == first.start_line && old_pos == first.start_pos)
		{
			// The cursor sits on the beginning of the range
			first.start_line = new_line;
			first.start_pos = new_pos;
		}
		else if (old_line == first.end_line && old_pos == first.end_pos)
		{
			// The cursor sits just past the end of the range
			first.end_line = new_line;
			first.end_pos = new_pos;
		}
		else
		{
			// The cursor has lost the range, which cannot normally happen
			// while the selection is in progress: a new one is started
			keeps_making = false;
		}
	}

	if (!keeps_making)
	{
		// The previous selection is replaced: the new range spans the
		// characters the movement has passed over
		if (!selected_ranges.empty ())
		{
			selected_ranges.erase (selected_ranges.begin ());
		}

		selection_finalized = false;

		selected_ranges.insert (selected_ranges.begin (), pos_less (new_line, new_pos, old_line, old_pos)
			? SelectedRange { new_line, new_pos, old_line, old_pos }
			: SelectedRange { old_line, old_pos, new_line, new_pos });
	}
}

void FileEditor::finalize_selection ()
{
	selection_finalized = true;

	// An empty range selects nothing: it is dropped from the list
	if (!selected_ranges.empty ())
	{
		auto &first = selected_ranges.front ();

		if (pos_less (first.end_line, first.end_pos, first.start_line, first.start_pos))
		{
			selected_ranges.erase (selected_ranges.begin ());
		}
	}
}

void FileEditor::begin_mouse_drag (int *top, int *left, int tab_width, ::Wenv::Display::Pos position)
{
	auto line = *top + position.y;
	auto column = *left + position.x;

	mouse_selecting = true;
	drag_anchor_line = line;
	drag_anchor_pos = file_line_raw_pos (get_file (), line, column, tab_width);
}

bool FileEditor::mouse_press_selection (int modifiers, int *top, int *left, int tab_width, ::Wenv::Display::Pos position)
{
	// The modifier flag of the shift key (see Window::current_mods)
	constexpr int shift = 2;

	// The shift-assisted press is not answered here: in the editing mode it
	// extends the selection the usual way, in the viewing mode there is no
	// cursor to extend from
	if (modifiers & shift)
	{
		return false;
	}

	if (!selected_ranges.empty () && selection_finalized)
	{
		// The click clears the finished selection; the drag that may follow
		// it creates a new one starting from the pressed character
		clear_selection ();
		begin_mouse_drag (top, left, tab_width, position);

		return true;
	}

	if (selected_ranges.empty ())
	{
		// The press starts the drag selection with the single character
		// under the mouse
		begin_mouse_drag (top, left, tab_width, position);
		selected_ranges.push_back ({ drag_anchor_line, drag_anchor_pos, drag_anchor_line, drag_anchor_pos + 1 });

		return true;
	}

	// The keyboard selection still in progress is left alone
	return false;
}

bool FileEditor::handle_mouse_drag (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers)
{
	// Only the drag started by the own press of the left button is answered
	if (!mouse_selecting || current_context == nullptr)
	{
		return false;
	}

	auto full_path = get_full_path ();

	if (full_path.empty ())
	{
		return false;
	}

	auto top = get_file_top_line (full_path);
	auto left = get_file_left_column (full_path);
	auto tab_width = *get_tab_width (current_display->get_persistent_context ());

	auto line = *top + position.y;
	auto column = *left + position.x;
	auto pos = file_line_raw_pos (get_file (), line, column, tab_width);

	// The selection spans the characters from the anchor one to the one
	// under the mouse; when the mouse has moved before the anchor, the far
	// end of the range becomes its beginning
	SelectedRange range = pos_less (line, pos, drag_anchor_line, drag_anchor_pos)
		? SelectedRange { line, pos, drag_anchor_line, drag_anchor_pos + 1 }
		: SelectedRange { drag_anchor_line, drag_anchor_pos, line, pos + 1 };

	// The first movement of the drag that has started by clearing the
	// finished selection creates the range, the following ones move its
	// mouse end
	if (selected_ranges.empty ())
	{
		selected_ranges.push_back (range);
	}
	else
	{
		selected_ranges[0] = range;
	}

	// In the editing mode the cursor follows the mouse as well
	if (*get_is_editing ())
	{
		auto cursor_line = get_file_cursor_line (full_path);
		auto cursor_pos = get_file_cursor_pos (full_path);

		*cursor_line = line;
		*cursor_pos = pos;
	}

	// The redraw shows the moved end of the selection band
	redraw_all (*get_focused_path ());

	return true;
}

// The iterator to the given line of the file; the line must exist
static std::list<FileLine>::iterator line_iterator (File *file, int line)
{
	auto it = file->lines.begin ();
	std::advance (it, line);
	return it;
}

// A character of a raw line: its byte offset and whether it is a word
// character (a unicode letter or a digit)
struct LineChar
{
	int pos;
	bool word;
};

// True when the utf-8 character in the given byte range is a word character:
// a unicode letter or a digit. The Windows character type table is used, so
// the letters outside the ASCII range are recognized as well.
static bool is_word_char (const std::string &raw, int pos, int length)
{
	auto text = maxy::strings::utf8towchar (raw.substr (pos, length));

	if (text.empty ())
	{
		return false;
	}

	std::vector<WORD> types (text.size (), 0);

	if (!GetStringTypeW (CT_CTYPE1, text.data (), (int) text.size (), types.data ()))
	{
		return false;
	}

	for (auto type : types)
	{
		if (type & (C1_ALPHA | C1_DIGIT))
		{
			return true;
		}
	}

	return false;
}

// Split the raw line into its characters, marking the word characters
static std::vector<LineChar> line_chars (const std::string &raw)
{
	std::vector<LineChar> chars;

	for (int pos = 0; pos < (int) raw.size (); )
	{
		auto length = utf8_char_length (raw[pos]);

		chars.push_back ({ pos, is_word_char (raw, pos, length) });
		pos += length;
	}

	return chars;
}

// The index of the character starting at or before the given raw position;
// -1 when the position is before the first character
static int char_index_at (const std::vector<LineChar> &chars, int pos)
{
	auto index = -1;

	for (int i = 0; i < (int) chars.size () && chars[i].pos <= pos; i++)
	{
		index = i;
	}

	return index;
}

// The index of the first character of the word that contains the given
// character index, which must be a word character
static int word_start_index (const std::vector<LineChar> &chars, int index)
{
	while (index > 0 && chars[index - 1].word)
	{
		index--;
	}

	return index;
}

void FileEditor::move_word_left (int *cursor_line, int *cursor_pos)
{
	auto file = get_file ();

	if (file == nullptr || file->lines.empty ())
	{
		return;
	}

	// The cursor is kept within the loaded content
	*cursor_line = max (0, min (*cursor_line, (int) file->lines.size () - 1));

	auto it = line_iterator (file, *cursor_line);
	auto chars = line_chars (it->raw_data);

	// The character just before the cursor: a cursor past the end of the
	// line starts at its last character
	auto index = char_index_at (chars, *cursor_pos - 1);

	while (true)
	{
		// The nearest word character to the left
		while (index >= 0 && !chars[index].word)
		{
			index--;
		}

		if (index >= 0)
		{
			// The cursor goes to the first character of the found word
			*cursor_pos = chars[word_start_index (chars, index)].pos;
			return;
		}

		// No word on this line: the search continues at the end of the
		// previous line, stopping at the beginning of the file
		if (*cursor_line == 0)
		{
			*cursor_pos = 0;
			return;
		}

		(*cursor_line)--;
		it = std::prev (it);
		chars = line_chars (it->raw_data);
		index = (int) chars.size () - 1;
	}
}

void FileEditor::move_word_right (int *cursor_line, int *cursor_pos)
{
	auto file = get_file ();

	if (file == nullptr || file->lines.empty ())
	{
		return;
	}

	// The cursor is kept within the loaded content
	*cursor_line = max (0, min (*cursor_line, (int) file->lines.size () - 1));

	auto it = line_iterator (file, *cursor_line);
	auto pos = max (*cursor_pos, 0);

	// The first line is searched from the cursor, the following ones from
	// their beginning
	bool from_cursor = true;

	while (true)
	{
		auto chars = line_chars (it->raw_data);
		auto raw_len = (int) it->raw_data.size ();

		// The index the search starts at. The current word is skipped when
		// the cursor is on one of its characters, so the next word is the
		// one after it; a cursor past the line end has nothing to search.
		auto start = 0;

		if (from_cursor)
		{
			auto under = char_index_at (chars, pos);

			if (pos >= raw_len)
			{
				start = (int) chars.size ();
			}
			else if (under >= 0 && chars[under].word)
			{
				start = under + 1;
			}
			else
			{
				start = max (under, 0);
			}
		}

		// The first character of the nearest word to the right
		for (int i = start; i < (int) chars.size (); i++)
		{
			if (chars[i].word && (i == 0 || !chars[i - 1].word))
			{
				*cursor_pos = chars[i].pos;
				return;
			}
		}

		// No word on this line: the search continues at the beginning of
		// the next line, stopping at the end of the file
		if (*cursor_line >= (int) file->lines.size () - 1)
		{
			*cursor_pos = raw_len;
			return;
		}

		(*cursor_line)++;
		it = std::next (it);
		from_cursor = false;
	}
}

void FileEditor::record_undo (UndoOperation op)
{
	// An edit made while the stack is below the saved state makes that
	// state unreachable by undoing: the marker goes to -1, so the file
	// stays edited until it is saved again
	if ((int) undo_stack.size () < saved_undo_stack_size)
	{
		saved_undo_stack_size = -1;
	}

	undo_stack.push_back (op);
}

// True when the byte is a utf-8 continuation byte (10xxxxxx)
static bool is_utf8_continuation (char byte)
{
	return ((unsigned char) byte & 0xC0) == 0x80;
}

// The unicode characters produced by the typed key (at most eight of them);
// false for the keys producing no text: the shortcuts with ctrl or alt held
// and the dead keys
static bool typed_chars (unsigned int key, int modifiers, wchar_t *chars, int *converted)
{
	// Keys with ctrl or alt held are shortcuts, not text input
	constexpr int ctrl = 1;
	constexpr int alt = 4;

	if (modifiers & ctrl || modifiers & alt)
	{
		return false;
	}

	// The key to character conversion uses the shift state of the modifiers
	BYTE keys[256] = {};
	if (modifiers & 2)
	{
		keys[VK_SHIFT] = 0x80;
	}

	// Zero characters for keys that produce no text, negative for dead keys
	*converted = (int) ToUnicode (key, 0, keys, chars, 8, 0);

	return *converted > 0;
}

bool FileEditor::insert_typed_char (unsigned int key, int modifiers, int *cursor_line, int *cursor_pos)
{
	// The typed characters of the key
	wchar_t chars[8] = {};
	auto converted = 0;

	auto file = get_file ();

	if (file == nullptr || !typed_chars (key, modifiers, chars, &converted))
	{
		return false;
	}

	// The typed characters replace the characters under the cursor only in
	// the overwrite mode, only when the cursor is on an existing line before
	// its end (past the end the input appends, as in the insert mode) and
	// only for plain characters (a newline is inserted rather than replaces)
	auto overwrite = get_overwrite_mode ();
	bool replaces = overwrite != nullptr && *overwrite
		&& *cursor_line >= 0 && *cursor_line < (int) file->lines.size ()
		&& *cursor_pos >= 0 && *cursor_pos < file_line_length (file, *cursor_line);

	for (int i = 0; replaces && i < converted; i++)
	{
		if (chars[i] == L'\r' || chars[i] == L'\n')
		{
			replaces = false;
		}
	}

	if (replaces)
	{
		auto replace_line = *cursor_line;
		auto replace_pos = *cursor_pos;
		auto it = line_iterator (file, replace_line);

		// All the characters typed by this keypress form one replacement:
		// each character takes the place of the character under the cursor,
		// and the extra ones (typed past the line end) are just appended
		std::string inserted;
		std::string removed;

		for (int i = 0; i < converted; i++)
		{
			auto utf8 = maxy::strings::wchartoutf8 (std::wstring { chars[i] });
			auto pos = (size_t) *cursor_pos;

			if (pos < it->raw_data.size ())
			{
				auto length = utf8_char_length (it->raw_data[pos]);

				removed += it->raw_data.substr (pos, length);
				it->raw_data.erase (pos, length);
			}

			it->raw_data.insert (pos, utf8);
			inserted += utf8;
			*cursor_pos = (int) pos + (int) utf8.size ();
		}

		// The edited line may have become the longest one
		file->measure (*it);

		// The undo puts the replaced characters back in place of the inserted
		// ones
		record_undo
		({
			.type = UndoType::replace,
			.line = replace_line,
			.pos = replace_pos,
			.text = inserted,
			.removed = removed
		});

		return true;
	}

	// The beginning of the inserted text in the unmodified file: the old end
	// of the line when the line is padded, the old end of the file when the
	// lines are appended
	auto old_size = (int) file->lines.size ();
	auto start_line = 0;
	auto start_pos = 0;

	if (old_size > 0)
	{
		start_line = min (*cursor_line, old_size - 1);
		start_pos = *cursor_line < old_size
			? min (*cursor_pos, file_line_length (file, *cursor_line))
			: file_line_length (file, old_size - 1);
	}

	// The cursor past the end of file: empty lines are appended so that the
	// cursor line exists and is the last of them
	if (*cursor_line >= (int) file->lines.size ())
	{
		// The line before the appended ones gets the separator; the cursor
		// line is the new last line and has no newline yet
		if (!file->lines.empty ())
		{
			file->lines.back ().newline = file->newline;
		}

		while (*cursor_line >= (int) file->lines.size ())
		{
			file->lines.push_back ({ {}, file->newline });
		}

		file->lines.back ().newline.clear ();
	}

	auto it = line_iterator (file, *cursor_line);

	// The cursor past the end of the line: the line is padded with spaces up
	// to the position just before the cursor so that the inserted character
	// becomes the last character of the line
	if (*cursor_pos > (int) it->raw_data.size ())
	{
		it->raw_data.append (*cursor_pos - (int) it->raw_data.size (), ' ');
	}

	for (int i = 0; i < converted; i++)
	{
		auto ch = chars[i];

		if (ch == L'\r' || ch == L'\n')
		{
			// The enter key produces a newline: the line is split at the
			// cursor, the part after the cursor becomes the new line. The
			// original newline of the line stays with the new line, the
			// inserted separator is the actual newline of the file
			auto split = (size_t) min (*cursor_pos, (int) it->raw_data.size ());
			auto tail = it->raw_data.substr (split);
			auto tail_newline = it->newline;

			it->raw_data.resize (split);
			it->newline = file->newline;

			it = file->lines.insert (std::next (it), FileLine { tail, tail_newline });

			(*cursor_line)++;
			*cursor_pos = 0;
		}
		else
		{
			// The character is inserted as utf-8 just before the character
			// that was under the cursor
			auto utf8 = maxy::strings::wchartoutf8 (std::wstring { ch });
			auto pos = (size_t) min (*cursor_pos, (int) it->raw_data.size ());

			it->raw_data.insert (pos, utf8);
			*cursor_pos += (int) utf8.size ();
		}

		// The edited line may have become the longest one
		file->measure (*it);
	}

	// The undo operation removes everything the input has added: the line
	// padding, the appended empty lines and the characters themselves
	record_undo
	({
		.type = UndoType::remove,
		.start_line = start_line,
		.start_pos = start_pos,
		.end_line = *cursor_line,
		.end_pos = *cursor_pos
	});

	return true;
}

bool FileEditor::backspace_at_cursor (int *cursor_line, int *cursor_pos)
{
	auto file = get_file ();

	// Nothing to remove before the beginning of the file or beyond the
	// loaded content
	if (file == nullptr || file->lines.empty () || *cursor_line >= (int) file->lines.size ())
	{
		return false;
	}

	auto it = line_iterator (file, *cursor_line);
	auto pos = min (*cursor_pos, (int) it->raw_data.size ());

	// The cursor at the beginning of the line: the preceding newline is
	// removed, concatenating the line to the previous one
	if (pos == 0)
	{
		// The first line has no preceding newline
		if (*cursor_line == 0)
		{
			return false;
		}

		auto prev = std::prev (it);
		auto prev_length = (int) prev->raw_data.size ();

		// The newline of the merged line is the one of the joined line; the
		// removed separator of the previous line is restored by the undo
		prev->raw_data += it->raw_data;
		prev->newline = it->newline;
		file->lines.erase (it);

		// The merged line may have become the longest one
		file->measure (*prev);

		(*cursor_line)--;
		*cursor_pos = prev_length;

		// The undo operation inserts the removed newline back
		record_undo
		({
			.type = UndoType::insert,
			.line = *cursor_line,
			.pos = prev_length,
			.text = "\n"
		});

		return true;
	}

	// The character just before the cursor starts at the first byte that is
	// not a utf-8 continuation byte when scanning back from the cursor (at
	// most three of them may belong to one character)
	auto start = pos - 1;
	while (start > 0 && pos - start < 4 && is_utf8_continuation (it->raw_data[start]))
	{
		start--;
	}

	auto removed = it->raw_data.substr (start, pos - start);
	it->raw_data.erase (start, pos - start);
	*cursor_pos = start;

	// The undo operation inserts the removed character back
	record_undo
	({
		.type = UndoType::insert,
		.line = *cursor_line,
		.pos = start,
		.text = removed
	});

	return true;
}

bool FileEditor::delete_at_cursor (int *cursor_line, int *cursor_pos)
{
	auto file = get_file ();

	// Nothing to remove beyond the loaded content
	if (file == nullptr || file->lines.empty () || *cursor_line >= (int) file->lines.size ())
	{
		return false;
	}

	auto it = line_iterator (file, *cursor_line);
	auto pos = min (*cursor_pos, (int) it->raw_data.size ());

	// The character at the cursor: its utf-8 byte sequence is removed
	if (pos < (int) it->raw_data.size ())
	{
		auto length = utf8_char_length (it->raw_data[pos]);
		auto removed = it->raw_data.substr (pos, length);

		it->raw_data.erase (pos, length);
		*cursor_pos = pos;

		// The undo operation inserts the removed character back
		record_undo
		({
			.type = UndoType::insert,
			.line = *cursor_line,
			.pos = pos,
			.text = removed
		});

		return true;
	}

	// The cursor is at or past the end of the line: the newline of the line
	// is removed, concatenating the next line to this one
	if (*cursor_line < (int) file->lines.size () - 1)
	{
		auto next = std::next (it);

		// The newline of the merged line is the one of the joined line
		it->raw_data += next->raw_data;
		it->newline = next->newline;
		file->lines.erase (next);

		// The merged line may have become the longest one
		file->measure (*it);

		// The cursor stays at the junction of the concatenated lines
		*cursor_pos = pos;

		// The undo operation inserts the removed newline back
		record_undo
		({
			.type = UndoType::insert,
			.line = *cursor_line,
			.pos = pos,
			.text = "\n"
		});

		return true;
	}

	// The cursor is at the end of the last line of a fully loaded file: the
	// trailing newline of the file is represented by the trailing empty line,
	// removing it concatenates the file without the newline
	if (file->is_fully_loaded () && file->lines.size () >= 2 && it->raw_data.empty ())
	{
		auto prev = std::prev (it);
		auto prev_length = (int) prev->raw_data.size ();

		// The trailing newline of the file is the newline of the line
		// before the trailing empty line
		prev->newline.clear ();
		file->lines.erase (it);

		*cursor_line = (int) file->lines.size () - 1;
		*cursor_pos = prev_length;

		// The undo operation inserts the removed newline back
		record_undo
		({
			.type = UndoType::insert,
			.line = *cursor_line,
			.pos = prev_length,
			.text = "\n"
		});

		return true;
	}

	return false;
}

// The utf-8 text to the windows clipboard as CF_UNICODETEXT: the content is
// widened and its \n separators are replaced with the \r\n the clipboard
// format uses
static bool copy_to_clipboard (HWND window, const std::string &utf8)
{
	// The clipboard text: the widened content with the CRLF separators
	std::wstring text;

	for (auto ch : maxy::strings::utf8towchar (utf8))
	{
		if (ch == L'\n')
		{
			text += L'\r';
		}

		text += ch;
	}

	// The clipboard serves a single opener at a time: the failure leaves
	// the previous content in place
	if (!OpenClipboard (window))
	{
		return false;
	}

	auto copied = false;
	auto handle = GlobalAlloc (GMEM_MOVEABLE, (text.size () + 1) * sizeof (wchar_t));

	if (handle != nullptr)
	{
		auto data = (wchar_t *) GlobalLock (handle);

		if (data != nullptr)
		{
			memcpy (data, text.c_str (), (text.size () + 1) * sizeof (wchar_t));
			GlobalUnlock (handle);

			// On success the system owns the allocation, on failure it is
			// freed here
			copied = SetClipboardData (CF_UNICODETEXT, handle) != nullptr;
		}

		if (!copied)
		{
			GlobalFree (handle);
		}
	}

	CloseClipboard ();

	return copied;
}

// The text of the windows clipboard as utf-8; empty when there is no text
// on it
static std::string clipboard_text ()
{
	std::string text;

	// The clipboard serves a single opener at a time
	if (!OpenClipboard (nullptr))
	{
		return text;
	}

	// The data handle is owned by the clipboard, it is only read here
	auto handle = GetClipboardData (CF_UNICODETEXT);

	if (handle != nullptr)
	{
		auto data = (wchar_t *) GlobalLock (handle);

		if (data != nullptr)
		{
			text = maxy::strings::wchartoutf8 (std::wstring { data });
			GlobalUnlock (handle);
		}
	}

	CloseClipboard ();

	return text;
}

bool FileEditor::normalize_selection (int *start_line, int *start_pos, int *end_line, int *end_pos)
{
	auto file = get_file ();

	// Nothing to work with beyond the loaded content
	if (file == nullptr || selected_ranges.empty () || file->lines.empty ())
	{
		return false;
	}

	// The range oriented from the earlier end to the later one
	auto range = selected_ranges.front ();

	if (pos_less (range.end_line, range.end_pos, range.start_line, range.start_pos))
	{
		std::swap (range.start_line, range.end_line);
		std::swap (range.start_pos, range.end_pos);
	}

	// An empty range selects nothing
	if (range.start_line == range.end_line && range.start_pos == range.end_pos)
	{
		return false;
	}

	// The range is clamped to the loaded content
	auto size = (int) file->lines.size ();
	*start_line = max (0, min (range.start_line, size - 1));
	*end_line = max (*start_line, min (range.end_line, size - 1));
	*start_pos = max (0, min (range.start_pos, (int) line_iterator (file, *start_line)->raw_data.size ()));
	*end_pos = max (0, min (range.end_pos, (int) line_iterator (file, *end_line)->raw_data.size ()));

	// The clamping may have emptied the range
	if (*start_line == *end_line && *start_pos >= *end_pos)
	{
		return false;
	}

	return true;
}

std::string FileEditor::selection_text ()
{
	int start_line, start_pos, end_line, end_pos;

	if (!normalize_selection (&start_line, &start_pos, &end_line, &end_pos))
	{
		return {};
	}

	auto file = get_file ();
	auto it = line_iterator (file, start_line);

	// The tail of the first line, the whole lines between the range ends
	// and the head of the last one, joined with the newline characters
	std::string text;

	if (start_line == end_line)
	{
		text = it->raw_data.substr (start_pos, end_pos - start_pos);
	}
	else
	{
		auto end_it = line_iterator (file, end_line);

		text = it->raw_data.substr (start_pos);

		for (auto middle = std::next (it); middle != end_it; ++middle)
		{
			text += "\n" + middle->raw_data;
		}

		text += "\n" + end_it->raw_data.substr (0, end_pos);
	}

	return text;
}

bool FileEditor::copy_selection ()
{
	// The selected text in the internal utf-8 form
	auto text = selection_text ();

	// Nothing selected: nothing to copy
	if (text.empty ())
	{
		return false;
	}

	// The clipboard needs the owner window: opened without one, the system
	// would drop the data on the emptying of the clipboard
	if (current_display == nullptr || current_display->window == nullptr)
	{
		return false;
	}

	return copy_to_clipboard (current_display->window->hwnd, text);
}

bool FileEditor::insert_text (const std::string &utf8, int *cursor_line, int *cursor_pos)
{
	auto file = get_file ();

	// The empty text changes nothing
	if (file == nullptr || utf8.empty ())
	{
		return false;
	}

	// The \r characters of the clipboard text (the halves of its CRLF
	// pairs) do not belong to the file lines
	auto pasted = utf8;

	for (auto pos = pasted.find ('\r'); pos != std::string::npos; pos = pasted.find ('\r', pos))
	{
		pasted.erase (pos, 1);
	}

	// The beginning of the inserted text in the unmodified file: the old
	// end of the line when the line is padded, the old end of the file
	// when the lines are appended
	auto old_size = (int) file->lines.size ();
	auto start_line = 0;
	auto start_pos = 0;

	if (old_size > 0)
	{
		start_line = min (*cursor_line, old_size - 1);
		start_pos = *cursor_line < old_size
			? min (*cursor_pos, file_line_length (file, *cursor_line))
			: file_line_length (file, old_size - 1);
	}

	// The cursor past the end of file: empty lines are appended so that
	// the cursor line exists and is the last of them
	if (*cursor_line >= (int) file->lines.size ())
	{
		// The line before the appended ones gets the separator; the cursor
		// line is the new last line and has no newline yet
		if (!file->lines.empty ())
		{
			file->lines.back ().newline = file->newline;
		}

		while (*cursor_line >= (int) file->lines.size ())
		{
			file->lines.push_back ({ {}, file->newline });
		}

		file->lines.back ().newline.clear ();
	}

	auto it = line_iterator (file, *cursor_line);

	// The cursor past the end of the line: the line is padded with spaces
	// up to the cursor so that the text begins at the old end of the line
	if (*cursor_pos > (int) it->raw_data.size ())
	{
		it->raw_data.append (*cursor_pos - (int) it->raw_data.size (), ' ');
	}

	// The tail of the line goes after the inserted text, keeping the
	// original newline of the line
	auto split = (size_t) min (*cursor_pos, (int) it->raw_data.size ());
	auto tail = it->raw_data.substr (split);
	auto tail_newline = it->newline;
	it->raw_data.resize (split);

	auto line_index = *cursor_line;

	// The text is inserted chunk by chunk, the newlines in it split the
	// line; the separators created by the insertion are the actual newline
	// of the file, the last chunk line keeps the newline of the tail
	for (size_t chunk_start = 0; ; )
	{
		auto nl = pasted.find ('\n', chunk_start);
		auto chunk_end = nl == std::string::npos ? pasted.size () : nl;

		it->raw_data += pasted.substr (chunk_start, chunk_end - chunk_start);
		it->newline = nl == std::string::npos ? tail_newline : file->newline;

		// The edited line may have become the longest one
		file->measure (*it);

		// The cursor stands just past the inserted text
		*cursor_line = line_index;
		*cursor_pos = (int) it->raw_data.size ();

		if (nl == std::string::npos)
		{
			break;
		}

		// The empty line for the next chunk (the explicit FileLine value,
		// since a bare {} would bind to the initializer_list overload of
		// insert and insert nothing)
		it = file->lines.insert (std::next (it), FileLine {});
		line_index++;
		chunk_start = nl + 1;
	}

	// The tail goes after the inserted text
	it->raw_data += tail;
	file->measure (*it);

	// The undo operation removes everything the input has added: the line
	// padding, the appended empty lines and the text itself
	record_undo
	({
		.type = UndoType::remove,
		.start_line = start_line,
		.start_pos = start_pos,
		.end_line = *cursor_line,
		.end_pos = *cursor_pos
	});

	return true;
}

bool FileEditor::paste_clipboard (int *cursor_line, int *cursor_pos)
{
	// The text of the clipboard
	auto text = clipboard_text ();

	// Nothing on the clipboard: the key is not handled
	if (text.empty ())
	{
		return false;
	}

	// The selected fragment is replaced by the text: the deletion records
	// its own undo operation and lands the cursor at its place
	if (!selected_ranges.empty ())
	{
		delete_selection (cursor_line, cursor_pos);
	}

	// The text is inserted at the cursor place
	return insert_text (text, cursor_line, cursor_pos);
}

bool FileEditor::delete_selection (int *cursor_line, int *cursor_pos)
{
	// The text of the selection goes into the undo operation
	auto removed = selection_text ();

	int start_line, start_pos, end_line, end_pos;

	// Nothing selected: the plain single-character behavior applies
	if (!normalize_selection (&start_line, &start_pos, &end_line, &end_pos))
	{
		return false;
	}

	auto file = get_file ();
	auto it = line_iterator (file, start_line);

	// The tail of the first line and the head of the last one merge
	// together, the lines between them disappear; the merged line keeps
	// the newline of the last line of the range
	if (start_line == end_line)
	{
		it->raw_data.erase (start_pos, end_pos - start_pos);
	}
	else
	{
		auto end_it = line_iterator (file, end_line);

		it->raw_data.resize (start_pos);
		it->raw_data += end_it->raw_data.substr (end_pos);
		it->newline = end_it->newline;

		file->lines.erase (std::next (it), std::next (end_it));
	}

	// The edited line may have become the longest one
	file->measure (*it);

	// The cursor is left at the place of the removal
	*cursor_line = start_line;
	*cursor_pos = start_pos;

	// The selection is consumed by the deletion
	selected_ranges.erase (selected_ranges.begin ());
	selection_finalized = true;

	// The undo operation inserts the removed text back
	record_undo
	({
		.type = UndoType::insert,
		.line = start_line,
		.pos = start_pos,
		.text = removed
	});

	return true;
}

bool FileEditor::replace_selection (unsigned int key, int modifiers, int *cursor_line, int *cursor_pos)
{
	// No selection: the typed characters go to the cursor place instead
	if (selected_ranges.empty ())
	{
		return false;
	}

	wchar_t chars[8] = {};
	auto converted = 0;

	auto file = get_file ();

	if (file == nullptr || file->lines.empty () || !typed_chars (key, modifiers, chars, &converted))
	{
		return false;
	}

	// The range oriented from the earlier end to the later one
	auto range = selected_ranges.front ();

	if (pos_less (range.end_line, range.end_pos, range.start_line, range.start_pos))
	{
		std::swap (range.start_line, range.end_line);
		std::swap (range.start_pos, range.end_pos);
	}

	// A selection spanning several lines is replaced by its deletion
	// followed by the plain insertion: the single replace undo operation
	// does not span several lines, and neither does the typed text with a
	// newline in it
	bool multiline = range.start_line != range.end_line;

	for (int i = 0; !multiline && i < converted; i++)
	{
		multiline = chars[i] == L'\r' || chars[i] == L'\n';
	}

	if (multiline)
	{
		if (!delete_selection (cursor_line, cursor_pos))
		{
			return false;
		}

		return insert_typed_char (key, modifiers, cursor_line, cursor_pos);
	}

	// The single-line replacement: the selection content is erased and the
	// typed characters take its place, all on the same line
	auto line = max (0, min (range.start_line, (int) file->lines.size () - 1));
	auto it = line_iterator (file, line);

	auto start = max (0, min (range.start_pos, (int) it->raw_data.size ()));
	auto end = max (start, min (range.end_pos, (int) it->raw_data.size ()));

	std::string inserted;
	auto removed = it->raw_data.substr (start, end - start);

	it->raw_data.erase (start, end - start);

	for (int i = 0; i < converted; i++)
	{
		auto utf8 = maxy::strings::wchartoutf8 (std::wstring { chars[i] });

		it->raw_data.insert ((size_t) start + inserted.size (), utf8);
		inserted += utf8;
	}

	// The edited line may have become the longest one
	file->measure (*it);

	// The cursor stands just past the inserted characters
	*cursor_line = line;
	*cursor_pos = start + (int) inserted.size ();

	// The selection is consumed by the replacement
	selected_ranges.erase (selected_ranges.begin ());
	selection_finalized = true;

	// The undo puts the replaced characters back in place of the inserted
	// ones
	record_undo
	({
		.type = UndoType::replace,
		.line = line,
		.pos = start,
		.text = inserted,
		.removed = removed
	});

	return true;
}

bool FileEditor::undo_last (int *cursor_line, int *cursor_pos)
{
	if (undo_stack.empty ())
	{
		return false;
	}

	auto op = undo_stack.back ();
	undo_stack.pop_back ();

	if (op.type == UndoType::insert)
	{
		apply_undo_insert (op, cursor_line, cursor_pos);
	}
	else if (op.type == UndoType::replace)
	{
		apply_undo_replace (op, cursor_line, cursor_pos);
	}
	else
	{
		apply_undo_remove (op, cursor_line, cursor_pos);
	}

	return true;
}

void FileEditor::discard_changes ()
{
	// The undo operations move the cursor as they are applied, which must
	// not affect the actual cursor of the session
	int unused_line = 0;
	int unused_pos = 0;

	// The operations are applied from the most recent one down to the first
	// one, which unwinds the content to its original state
	while (undo_last (&unused_line, &unused_pos))
	{
	}

	update_pending_changes ();
}

void FileEditor::apply_undo_insert (const UndoOperation &op, int *cursor_line, int *cursor_pos)
{
	auto file = get_file ();

	if (file == nullptr)
	{
		return;
	}

	// Empty lines are appended so that the place of the insert exists
	while (op.line >= (int) file->lines.size ())
	{
		file->lines.push_back ({ {}, file->newline });
	}

	auto it = line_iterator (file, op.line);
	auto place = (size_t) max (0, min (op.pos, (int) it->raw_data.size ()));

	// The tail of the line goes after the inserted text, keeping the
	// original newline of the line
	auto tail = it->raw_data.substr (place);
	auto tail_newline = it->newline;
	it->raw_data.resize (place);

	auto line_index = op.line;
	auto end_line = op.line;
	auto end_pos = 0;

	// The text is inserted chunk by chunk, the newlines in it split the
	// line; the separators created by the insertion are the actual newline
	// of the file, the last chunk line keeps the newline of the tail
	for (size_t chunk_start = 0; ; )
	{
		auto nl = op.text.find ('\n', chunk_start);
		auto chunk_end = nl == std::string::npos ? op.text.size () : nl;

		it->raw_data += op.text.substr (chunk_start, chunk_end - chunk_start);
		it->newline = nl == std::string::npos ? tail_newline : file->newline;

		// The edited line may have become the longest one
		file->measure (*it);

		end_line = line_index;
		end_pos = (int) it->raw_data.size ();

		if (nl == std::string::npos)
		{
			break;
		}

		// The empty line for the next chunk: the explicit FileLine value,
		// since a bare {} would bind to the initializer_list overload of
		// insert and insert nothing
		it = file->lines.insert (std::next (it), FileLine {});
		line_index++;
		chunk_start = nl + 1;
	}

	it->raw_data += tail;
	file->measure (*it);

	// The cursor is placed just past the inserted text
	*cursor_line = end_line;
	*cursor_pos = end_pos;
}

void FileEditor::apply_undo_remove (const UndoOperation &op, int *cursor_line, int *cursor_pos)
{
	auto file = get_file ();

	if (file == nullptr || file->lines.empty ())
	{
		return;
	}

	// The range is clamped to the loaded content
	auto size = (int) file->lines.size ();
	auto start_line = max (0, min (op.start_line, size - 1));
	auto end_line = max (start_line, min (op.end_line, size - 1));

	auto it = line_iterator (file, start_line);
	auto start_pos = max (0, min (op.start_pos, (int) it->raw_data.size ()));

	if (start_line == end_line)
	{
		it->raw_data.erase (start_pos, max (0, op.end_pos - start_pos));
	}
	else
	{
		auto end_it = line_iterator (file, end_line);
		auto end_pos = max (0, min (op.end_pos, (int) end_it->raw_data.size ()));

		// The head of the first line and the tail of the last line of the
		// range merge together, the lines between them disappear; the
		// merged line keeps the newline of the last line of the range
		it->raw_data.resize (start_pos);
		it->raw_data += end_it->raw_data.substr (end_pos);
		it->newline = end_it->newline;

		file->lines.erase (std::next (it), std::next (end_it));

		// The merged line may have become the longest one
		file->measure (*it);
	}

	// The cursor is placed at the beginning of the removed range
	*cursor_line = start_line;
	*cursor_pos = start_pos;
}

void FileEditor::apply_undo_replace (const UndoOperation &op, int *cursor_line, int *cursor_pos)
{
	auto file = get_file ();

	if (file == nullptr || op.line < 0 || op.line >= (int) file->lines.size ())
	{
		return;
	}

	auto it = line_iterator (file, op.line);
	auto place = (size_t) max (0, min (op.pos, (int) it->raw_data.size ()));
	auto inserted_length = min ((int) op.text.size (), (int) it->raw_data.size () - (int) place);

	// The inserted text is removed and the original text is put back in its
	// place
	it->raw_data.erase (place, inserted_length);
	it->raw_data.insert (place, op.removed);

	// The line may have become the longest one
	file->measure (*it);

	// The cursor is placed at the beginning of the replacement
	*cursor_line = op.line;
	*cursor_pos = (int) place;
}

void FileEditor::save_changes ()
{
	auto file = get_file ();

	if (file != nullptr)
	{
		// The lines are written with their own newlines, so the lines
		// created during the editing are saved with the actual newline of
		// the file while the pre-existing ones keep theirs
		file->save ();

		// The saved content matches the current undo stack state: any
		// further edit or undo application makes the file edited again
		saved_undo_stack_size = (int) undo_stack.size ();
		update_pending_changes ();
	}
}

void FileEditor::show_guard_modal (const std::string &action)
{
	if (current_display == nullptr || current_context == nullptr)
	{
		return;
	}

	// The action to proceed with when the changes are saved or discarded
	current_context->set ("guard-action", new std::string { action });

	std::vector<::Wenv::Display::ModalButton> buttons =
	{
		{ L"Save the changes", guard_save },
		{ L"Discard the changes", guard_discard },
		{ L"Cancel", nullptr }
	};

	current_display->show_modal
	(
		"guard",
		L"Warning",
		L"There are unsaved changes, what do you want to do?",
		buttons,
		::Wenv::Display::Palette::Warning_element_color,
		::Wenv::Display::Palette::Warning_element_color,
		::Wenv::Display::Palette::Warning_element_color
	);
}

void FileEditor::redraw_all (const std::string &path)
{
	auto apps = get_app_group ();

	if (apps != nullptr)
	{
		for (auto app : *apps)
		{
			app->with_context (current_context)->redraw (path);
		}
	}
}

std::wstring * FileEditor::get_edit_target ()
{
	// No default can be provided for this parameter because it is set
	// from outside (the file manager) when the editor is opened
	return current_context->get<std::wstring> ("edit-target");
}

std::wstring * FileEditor::get_edit_pwd ()
{
	// No default can be provided for this parameter because it is set
	// from outside (the file manager) together with edit-target
	return current_context->get<std::wstring> ("edit-pwd");
}

std::wstring FileEditor::get_full_path ()
{
	auto target = get_edit_target ();
	auto pwd = get_edit_pwd ();

	// No target or working directory - no file is being edited
	if (target == nullptr || target->empty () || pwd == nullptr || pwd->empty ())
	{
		return {};
	}

	auto full_path = *pwd;

	if (full_path.back () != L'\\' && full_path.back () != L'/')
	{
		full_path += L"\\";
	}

	full_path += *target;

	return full_path;
}

std::wstring * FileEditor::get_viewed_path ()
{
	// No default: a missing value means no file has been viewed yet, which
	// makes the first redraw load the requested file
	return current_context->get<std::wstring> ("viewed-path");
}

File * FileEditor::get_file ()
{
	// No default: the file is loaded and stored by the editor itself
	return current_context->get<File> ("file");
}

App * FileEditor::get_status_bar ()
{
	// No default: an absent status bar simply leaves nothing to refresh
	return current_context->get<App> ("status-bar");
}

std::string * FileEditor::get_focused_path ()
{
	// Set by the core when the display is built; no default makes sense here
	return current_context->get<std::string> ("focused-path");
}

std::vector<App *> * FileEditor::get_app_group ()
{
	// No default: a missing group simply means there are no apps to redraw
	return current_context->get<std::vector<App *>> ("app-group");
}

int * FileEditor::get_file_top_line (const std::wstring &full_path)
{
	// The per-file viewport is kept in the persistent context under a key
	// derived from the file path; a freshly opened file starts at the top
	auto per_file_key = std::string { "top-line:" } + maxy::strings::wchartoutf8 (full_path);

	return current_display->get_persistent_context ()->get<int> (per_file_key, [] () { return new int { 0 }; });
}

int * FileEditor::get_file_left_column (const std::wstring &full_path)
{
	// A freshly opened file starts at the leftmost column
	auto per_file_key = std::string { "left-col:" } + maxy::strings::wchartoutf8 (full_path);

	return current_display->get_persistent_context ()->get<int> (per_file_key, [] () { return new int { 0 }; });
}

bool * FileEditor::get_is_editing ()
{
	// Default: a file that has just been opened is viewed, not edited. The
	// state is set from outside (the file manager or the F6 command)
	return current_context->get<bool> ("is_editing", [] () { return new bool { false }; });
}

bool * FileEditor::get_overwrite_mode ()
{
	// Default: the insert mode. The state is reset whenever a file is opened
	// for editing or the editing mode is toggled
	return current_context->get<bool> ("overwrite-mode", [] () { return new bool { false }; });
}

bool * FileEditor::get_pending_changes ()
{
	// Default: a freshly opened file has no unsaved changes. The flag is
	// shared with the status bar, which shows the unsaved changes marker
	// by it, so the stack itself stays hidden inside the editor
	return current_context->get<bool> ("pending-changes", [] () { return new bool { false }; });
}

void FileEditor::update_pending_changes ()
{
	if (current_context == nullptr)
	{
		return;
	}

	// The file counts as edited while the undo stack size differs from the
	// size it had when the file was saved last
	*get_pending_changes () = (int) undo_stack.size () != saved_undo_stack_size;
}

int * FileEditor::get_file_cursor_line (const std::wstring &full_path)
{
	// The per-file cursor position is kept in the persistent context under
	// keys derived from the file path; a freshly opened file starts with the
	// cursor on the first character of the first line
	auto per_file_key = std::string { "cursor-line:" } + maxy::strings::wchartoutf8 (full_path);

	return current_display->get_persistent_context ()->get<int> (per_file_key, [] () { return new int { 0 }; });
}

int * FileEditor::get_file_cursor_pos (const std::wstring &full_path)
{
	// The cursor position is kept in the raw coordinates of the line, before
	// the tab expansion
	auto per_file_key = std::string { "cursor-pos:" } + maxy::strings::wchartoutf8 (full_path);

	return current_display->get_persistent_context ()->get<int> (per_file_key, [] () { return new int { 0 }; });
}

}

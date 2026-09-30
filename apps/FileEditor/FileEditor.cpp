#include <Windows.h>
#include <iterator>
#include <string>
#include <vector>
#include "../../display/Display.h"
#include "../../display/Window.h"
#include "../../display/Palette.h"
#include "../../maxy/strings.h"
#include "../../Context.h"
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

	if (display == nullptr || display->window == nullptr)
	{
		return;
	}

	display->window->close_current_display ();
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
	auto cursor_line = get_file_cursor_line (full_path);
	auto cursor_pos = get_file_cursor_pos (full_path);

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

		// Draw trailing spaces as middle dots
		if (!expanded.empty ())
		{
			auto last_non_space = expanded.find_last_not_of (L' ');

			// An all-whitespace line is all trailing spaces
			int trailing_start = last_non_space == std::wstring::npos ? 0 : (int) last_non_space + 1;
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
	}

	// The text cursor is hidden in the viewing mode; in the editing mode it is
	// shown at the position computed from the cursor position in the file and
	// the currently visible portion of it
	current_display->is_cursor_visible = *is_editing;

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

	// The cursor is only moved in the editing mode
	if (!*get_is_editing ())
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

	auto line = *top + position.y;
	auto column = *left + position.x;

	auto cursor_line = get_file_cursor_line (full_path);
	auto cursor_pos = get_file_cursor_pos (full_path);

	*cursor_line = line;
	*cursor_pos = file_line_raw_pos (get_file (), line, column, tab_width);

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

	// The Escape key is owned by the editor: in the editing mode it is
	// ignored so the unsaved changes cannot be lost, in the viewing mode
	// the editor is closed (without saving)
	if (key == VK_ESCAPE)
	{
		if (!*is_editing)
		{
			editor_exit (current_display, current_context);
		}

		return true;
	}

	if (*is_editing)
	{
		// The editing mode: the keys move the cursor in the raw coordinates
		// of the file, the viewport follows its display position
		auto cursor_line = get_file_cursor_line (full_path);
		auto cursor_pos = get_file_cursor_pos (full_path);

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
			(*cursor_pos)--;
		}
		else if (key == VK_RIGHT)
		{
			(*cursor_pos)++;
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
		else if (key == VK_BACK)
		{
			// Backspace removes the character just before the cursor
			handled = backspace_at_cursor (cursor_line, cursor_pos);
		}
		else if (key == VK_DELETE)
		{
			// Delete removes the character at the cursor
			handled = delete_at_cursor (cursor_line, cursor_pos);
		}
		else if (key == 'Z' && (modifiers & ctrl))
		{
			// Ctrl+Z applies and drops the last undo operation
			handled = undo_last (cursor_line, cursor_pos);
		}
		else
		{
			// A regular keypress: convert the key to its unicode character
			// and insert it into the text at the cursor position
			handled = insert_typed_char (key, modifiers, cursor_line, cursor_pos);
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

// The iterator to the given line of the file; the line must exist
static std::list<FileLine>::iterator line_iterator (File *file, int line)
{
	auto it = file->lines.begin ();
	std::advance (it, line);
	return it;
}

// True when the byte is a utf-8 continuation byte (10xxxxxx)
static bool is_utf8_continuation (char byte)
{
	return ((unsigned char) byte & 0xC0) == 0x80;
}

bool FileEditor::insert_typed_char (unsigned int key, int modifiers, int *cursor_line, int *cursor_pos)
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

	wchar_t chars[8] = {};
	auto converted = ToUnicode (key, 0, keys, chars, (int) std::size (chars), 0);

	// Zero characters for keys that produce no text, negative for dead keys
	if (converted <= 0)
	{
		return false;
	}

	auto file = get_file ();

	if (file == nullptr)
	{
		return false;
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

			it = file->lines.insert (std::next (it), { tail, tail_newline });

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
	undo_stack.push_back
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
		undo_stack.push_back
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
	undo_stack.push_back
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
		undo_stack.push_back
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
		undo_stack.push_back
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
		undo_stack.push_back
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

		it = file->lines.insert (std::next (it), {});
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

void FileEditor::save_changes ()
{
	auto file = get_file ();

	if (file != nullptr)
	{
		// The lines are written with their own newlines, so the lines
		// created during the editing are saved with the actual newline of
		// the file while the pre-existing ones keep theirs
		file->save ();
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

	*get_pending_changes () = !undo_stack.empty ();
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

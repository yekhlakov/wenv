#include <Windows.h>
#include <format>
#include "../../maxy/strings.h"
#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../../Context.h"
#include "File.h"
#include "FileEditorStatusBar.h"

namespace Wenv::Apps
{

void FileEditorStatusBar::draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area)
{
	App::draw (display, path, client_area);

	if (current_context == nullptr)
	{
		return;
	}

	redraw (path);
}

void FileEditorStatusBar::redraw (const std::string &path)
{
	if (current_display == nullptr || current_context == nullptr)
	{
		return;
	}

	auto area = get_client_area (path);
	auto target = get_edit_target ();
	auto pwd = get_edit_pwd ();
	auto file = get_file ();
	auto is_editing = get_is_editing ();

	if (target == nullptr || pwd == nullptr)
	{
		return;
	}

	auto full_path = *pwd;
	if (full_path.back () != L'\\' && full_path.back () != L'/')
	{
		full_path += L"\\";
	}
	full_path += *target;

	auto top = get_file_top_line (full_path);
	auto left = get_file_left_column (full_path);

	// The tab width is a persistent setting, shared with the editor
	auto tab_width = *get_tab_width (current_display->get_persistent_context ());

	// The right-aligned information is composed of text parts, the ones
	// flagged true are drawn in the dark color
	std::vector<std::pair<std::wstring, bool>> parts;

	if (file != nullptr)
	{
		auto line_count = file->get_line_count ();
		auto loaded_count = (int) file->lines.size ();

		// The newline of the file as determined on opening
		auto newline_part = file->newline == "\n" ? L"  LF" : L"  CRLF";

		// The total line count part: the real count of a fully loaded file,
		// a question mark otherwise
		auto count_part = line_count != UNKNOWN_LINE_COUNT
			? std::format (L"/{}", line_count)
			: L"/?";

		if (*is_editing)
		{
			auto cursor_line = get_file_cursor_line (full_path);
			auto cursor_pos = get_file_cursor_pos (full_path);

			// The position of the cursor in display coordinates
			auto display_pos = file_line_display_pos (file, *cursor_line, *cursor_pos, tab_width);

			// The cursor past the last line of a fully loaded file or past
			// the end of its current line makes the number dark
			auto past_last_line = line_count != UNKNOWN_LINE_COUNT && *cursor_line >= line_count;
			auto past_line_end = *cursor_pos > file_line_length (file, *cursor_line);

			parts =
			{
				{ std::format (L"  {} B  ", file->get_file_size ()), false },
				{ std::format (L"{}", *cursor_line + 1), past_last_line },
				{ count_part, false },
				{ L"  Col ", false },
				{ std::format (L"{}", display_pos + 1), past_line_end },
				{ std::format (L"  T{}", tab_width), false },
				{ newline_part, false }
			};
		}
		else
		{
			// The viewing mode shows the position of the viewport
			auto line_num = min (*top + 1, loaded_count);

			parts =
			{
				{ std::format (L"  {} B  ", file->get_file_size ()), false },
				{ std::format (L"{}", line_num), false },
				{ count_part, false },
				{ std::format (L"  Col {}", *left + 1), false },
				{ std::format (L"  T{}", tab_width), false },
				{ newline_part, false }
			};
		}
	}

	// The whole bar is drawn in the highlight variant of its colors; the mark
	// in the leftmost corner shows the state of the editor: an open circle of
	// the quote color when the file is only viewed and a filled one of the
	// warning color when it is edited. Both are single cells of the font, so
	// the name of the file follows right after the mark
	auto mark = *is_editing ? L"\u25CF" : L"\u25CB";

	current_display->with_color (::Wenv::Display::Palette::Default_color, true);
	current_display->print_line (area, L"", current_display->PF_ERASE_BACKGROUND);

	current_display->with_color
	(
		*is_editing
			? ::Wenv::Display::Palette::Warning_element_color
			: ::Wenv::Display::Palette::Quote_element_color,
		true
	);

	current_display->print_line (area.x, area.y, mark);

	// The name of an edited file is colored like its mark
	if (*is_editing)
	{
		current_display->with_color (::Wenv::Display::Palette::Warning_element_color, true);
	}

	current_display->print_line (area.x + 1, area.y, full_path);

	// The unsaved changes marker follows the file name when the editor
	// keeps undo operations for the file; the editor propagates the state
	// of its undo stack through this context flag
	auto pending = current_context->get<bool> ("pending-changes", [] () { return new bool { false }; });

	if (*pending)
	{
		current_display->with_color (::Wenv::Display::Palette::Active_element_color, true);
		current_display->print_line (area.x + 1 + (int) full_path.size (), area.y, L"*");
	}

	// The parts are drawn sequentially from the right edge of the bar
	auto total_width = 0;

	for (auto &part : parts)
	{
		total_width += (int) part.first.size ();
	}

	auto x = area.x + area.width - total_width;

	for (auto &[text, dark] : parts)
	{
		current_display->with_color
		(
			dark ? ::Wenv::Display::Palette::Dark_element_color : ::Wenv::Display::Palette::Default_color,
			true
		);

		current_display->print_line (x, area.y, text);
		x += (int) text.size ();
	}
}

std::wstring * FileEditorStatusBar::get_edit_target ()
{
	// No default can be provided for this parameter because it is set
	// from outside (the file manager) when the editor is opened
	return current_context->get<std::wstring> ("edit-target");
}

std::wstring * FileEditorStatusBar::get_edit_pwd ()
{
	// No default can be provided for this parameter because it is set
	// from outside (the file manager) together with edit-target
	return current_context->get<std::wstring> ("edit-pwd");
}

File * FileEditorStatusBar::get_file ()
{
	// No default: the file is loaded and stored by the editor itself; the
	// bar only appends the file details when one is actually loaded
	return current_context->get<File> ("file");
}

bool * FileEditorStatusBar::get_is_editing ()
{
	// Default: a file that has just been opened is viewed, not edited. The
	// state is set from outside (the file manager) when the file is shown
	return current_context->get<bool> ("is_editing", [] () { return new bool { false }; });
}

int * FileEditorStatusBar::get_file_top_line (const std::wstring &full_path)
{
	// The per-file viewport is kept in the persistent context under a key
	// derived from the file path; a freshly opened file starts at the top
	auto per_file_key = std::string { "top-line:" } + maxy::strings::wchartoutf8 (full_path);

	return current_display->get_persistent_context ()->get<int> (per_file_key, [] () { return new int { 0 }; });
}

int * FileEditorStatusBar::get_file_left_column (const std::wstring &full_path)
{
	// A freshly opened file starts at the leftmost column
	auto per_file_key = std::string { "left-col:" } + maxy::strings::wchartoutf8 (full_path);

	return current_display->get_persistent_context ()->get<int> (per_file_key, [] () { return new int { 0 }; });
}

int * FileEditorStatusBar::get_file_cursor_line (const std::wstring &full_path)
{
	// The per-file cursor position is kept in the persistent context under
	// keys derived from the file path (see the editor)
	auto per_file_key = std::string { "cursor-line:" } + maxy::strings::wchartoutf8 (full_path);

	return current_display->get_persistent_context ()->get<int> (per_file_key, [] () { return new int { 0 }; });
}

int * FileEditorStatusBar::get_file_cursor_pos (const std::wstring &full_path)
{
	// The cursor position is kept in the raw coordinates of the line, before
	// the tab expansion
	auto per_file_key = std::string { "cursor-pos:" } + maxy::strings::wchartoutf8 (full_path);

	return current_display->get_persistent_context ()->get<int> (per_file_key, [] () { return new int { 0 }; });
}

}

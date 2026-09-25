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

	// Right-aligned information
	std::wstring right;
	if (file != nullptr)
	{
		auto line_count = file->get_line_count ();
		auto loaded_count = (int) file->lines.size ();
		auto line_num = min (*top + 1, loaded_count);

		if (line_count == UNKNOWN_LINE_COUNT)
		{
			right = std::format
			(
				L"  {}  {}/?  Col {}  T{}",
				std::format (L"{} B", file->get_file_size ()),
				line_num,
				*left + 1,
				tab_width
			);
		}
		else
		{
			right = std::format
			(
				L"  {}  {}/{}  Col {}  T{}",
				std::format (L"{} B", file->get_file_size ()),
				line_num,
				line_count,
				*left + 1,
				tab_width
			);
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

	current_display->with_color (::Wenv::Display::Palette::Default_color, true);

	current_display->print_line
	(
		area,
		right,
		current_display->PF_TOP | current_display->PF_RIGHT
	);
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

}

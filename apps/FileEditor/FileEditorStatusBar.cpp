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
				L"  {}  {}/?  Col {}",
				std::format (L"{} B", file->get_file_size ()),
				line_num,
				*left + 1
			);
		}
		else
		{
			right = std::format
			(
				L"  {}  {}/{}  Col {}",
				std::format (L"{} B", file->get_file_size ()),
				line_num,
				line_count,
				*left + 1
			);
		}
	}

	current_display->with_color (::Wenv::Display::Palette::Default_color, true);

	current_display->print_line
	(
		area,
		full_path,
		current_display->PF_TOP | current_display->PF_LEFT | current_display->PF_ERASE_BACKGROUND
	);

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

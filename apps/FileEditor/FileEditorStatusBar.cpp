#include <Windows.h>
#include <format>
#include "../../maxy/strings.h"
#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../Context.h"
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
	auto target = current_context->get<std::wstring> ("edit-target");
	auto pwd = current_context->get<std::wstring> ("edit-pwd");
	auto file = current_context->get<File> ("file");
	auto top = current_context->get<int> ("top-line", [] () { return new int {}; });
	auto left = current_context->get<int> ("left-col", [] () { return new int {}; });

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

	current_display->with_color (::Wenv::Display::Palette::Highlight_color);

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

}

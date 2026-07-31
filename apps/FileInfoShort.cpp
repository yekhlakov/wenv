#include <format>
#include <vector>
#include <Windows.h>
#include "..\display\Display.h"
#include "..\display\Palette.h"
#include "Context.h"
#include "FileInfoShort.h"

namespace Wenv::Apps
{


int *get_selected_file_idx (Context *c);

void FileInfoShort::draw (::Wenv::Display::Display &display, ::Wenv::Display::Rect client_area)
{
	App::draw (display, client_area);

	if (current_context == nullptr)
	{
		return;
	}

	redraw ();
}

void FileInfoShort::redraw ()
{
	auto lst = current_context->get<std::vector<WIN32_FIND_DATAW>> ("sorted-list");
	auto selected_file_idx = get_selected_file_idx (current_context);

	auto fd = (*lst)[*selected_file_idx];

	SYSTEMTIME st;
	FileTimeToSystemTime (&fd.ftLastWriteTime, &st);
	SYSTEMTIME lt;
	SystemTimeToTzSpecificLocalTime (nullptr, &st, &lt);

	bool is_directory = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

	auto fn = std::wstring { fd.cFileName };

	current_display->with_color(::Wenv::Display::Palette::Default_color)->print_line
	(
		current_client_area,
		fn + (is_directory ? L"/" : L""),
		current_display->PF_LEFT | current_display->PF_ERASE_BACKGROUND
	);

	std::wstring i = fn == L".." ? L"Up" :
		is_directory ? L"Dir" : 
		std::format (L"{}", (((long long) fd.nFileSizeHigh) << 32) + (long long) fd.nFileSizeLow);

	current_display->print_line
	(
		current_client_area,
		std::format(L"{}  {:4}-{:02}-{:02} {:02}:{:02}", i, lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute),
		current_display->PF_RIGHT
	);
}

}
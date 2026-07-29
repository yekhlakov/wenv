#include <algorithm>
#include <Windows.h>
#include "../maxy/strings.h"
#include "../display/Display.h"
#include "../display/Palette.h"
#include "FileList.h"
#include "Context.h"

namespace Wenv::Apps
{

std::vector<WIN32_FIND_DATAW> *list_directory_contents (const std::wstring &dirname)
{
	auto v = new std::vector<WIN32_FIND_DATAW> {};

	std::wstring searchPath = dirname + L"\\*";

	WIN32_FIND_DATAW findData;
	HANDLE hFind = FindFirstFileW (searchPath.c_str (), &findData);

	if (hFind == INVALID_HANDLE_VALUE) {
		return v;
	}

	do {
		std::wstring fileName = findData.cFileName;

		// Skip current directory dot "."
		if (fileName == L".") {
			continue;
		}

		v->push_back (findData);

	} while (FindNextFileW (hFind, &findData) != 0); // Fetch next item

	// Clean up the search handle resource
	FindClose (hFind);

	return v;
}

std::vector<WIN32_FIND_DATAW> sort_file_list (std::vector<WIN32_FIND_DATAW> *v, int sort_mode)
{
	
	std::vector<WIN32_FIND_DATAW> dirs {};
	std::vector<WIN32_FIND_DATAW> files {};
	std::vector<WIN32_FIND_DATAW> output {};
	for (auto &f : *v)
	{
		if (std::wstring { f.cFileName } == L"..")
		{
			// `..` directory is always first regardless of sort mode
			output.push_back (f);

			continue;
		}

		if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			dirs.push_back (f);
		}
		else
		{
			files.push_back (f);
		}
	}

	// Default sort: alphabetical, directories first

	auto sorter = 
		// reverse alphabetical
		sort_mode == 1 ? [] (WIN32_FIND_DATAW & a, WIN32_FIND_DATAW & b) { return std::wstring { a.cFileName } > std::wstring { b.cFileName }; } :
		// size 
		sort_mode == 2 ? [] (WIN32_FIND_DATAW &a, WIN32_FIND_DATAW &b) {
			return a.nFileSizeHigh < b.nFileSizeHigh || a.nFileSizeHigh == b.nFileSizeHigh && a.nFileSizeLow < b.nFileSizeLow;
		} :
		// reverse size
		sort_mode == 3 ? [] (WIN32_FIND_DATAW &a, WIN32_FIND_DATAW &b) {
			return a.nFileSizeHigh > b.nFileSizeHigh || a.nFileSizeHigh == b.nFileSizeHigh && a.nFileSizeLow > b.nFileSizeLow;
		} :
		// default: alphabetical
		[] (WIN32_FIND_DATAW &a, WIN32_FIND_DATAW &b) { return std::wstring { a.cFileName } < std::wstring { b.cFileName }; }
	;

	std::sort (dirs.begin (), dirs.end (), sorter);
	std::sort (files.begin (), files.end (), sorter);


	// Combine .., other directories, and files
	output.reserve (1 + dirs.size () + files.size ());
	std::ranges::copy (dirs, std::back_inserter (output));
	std::ranges::copy (files, std::back_inserter (output));

	return output;
}

int *get_selected_file_idx (Context * c)
{
	return c->get<int> ("selected-file-idx", [] () ->int *{ return new int { 0 }; });
}

void FileList::draw (::Wenv::Display::Display &display, ::Wenv::Display::Rect client_area)
{
	App::draw (display, client_area);

	if (current_context == nullptr)
	{
		return;
	}

	redraw ();
}

void FileList::redraw()
{
	auto s = current_context->get<std::wstring> ("pwd");
	auto lst = current_context->get<std::vector<WIN32_FIND_DATAW>> ("list", [&] () {return list_directory_contents (*s); });
	auto selected_file_idx = get_selected_file_idx (current_context);

	auto sorted_lst = sort_file_list (lst, 0);

	*selected_file_idx = min ((int) lst->size () - 1, *selected_file_idx);
	*selected_file_idx = max (0, *selected_file_idx);

	::Wenv::Display::Rect fn_rect = current_client_area;
	fn_rect.height = 1;

	int p = 0;
	int p_begin = 0;
	if (name[0] == L'2')
	{
		// This is the right column so we must skip some of the first elements
		p_begin = current_client_area.height;
	}

	for (auto &fd : sorted_lst)
	{
		if (p < p_begin)
		{
			p++;
			continue;
		}

		if (p == *selected_file_idx)
		{
			current_display->with_color (::Wenv::Display::Palette::Highlight_color);
		}
		else
		{
			current_display->with_color (::Wenv::Display::Palette::Default_color);
		}

		fn_rect.y = current_client_area.y + p - p_begin;

		if (fn_rect.y - current_client_area.y >= current_client_area.height)
		{
			break;
		}

		current_display->print_line
		(
			fn_rect,
			std::wstring { fd.cFileName } + (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? L"/" : L""),
			current_display->PF_TOP | current_display->PF_LEFT | current_display->PF_ERASE_BACKGROUND
		);

		p++;
	}
}

void FileList::click (::Wenv::Display::Rect client_area, int modifiers)
{
}

void FileList::keypress (int key, int modifiers)
{
	auto idx = get_selected_file_idx (current_context);

	if (key == VK_DOWN)
	{
		(*idx)++;
	}
	else if (key == VK_UP)
	{
		*idx = max (0, (*idx) - 1);
	}

	auto cols = current_context->get<std::vector<App *>> ("columns");
	if (cols != nullptr)
	{
		for (auto app : *cols)
		{
			app->redraw ();
		}
	}
}

}

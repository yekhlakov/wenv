#include <algorithm>
#include <Shlwapi.h>
#include <Windows.h>
#include "../maxy/strings.h"
#include "../display/Display.h"
#include "../display/Palette.h"
#include "FileList.h"
#include "Context.h"

#pragma comment(lib, "shlwapi.lib")

namespace Wenv::Apps
{

using File_list_type = std::vector<WIN32_FIND_DATAW>;

File_list_type *list_directory_contents (const std::wstring &dirname)
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

File_list_type * sort_file_list (File_list_type *v, int sort_mode)
{	
	File_list_type dirs {};
	File_list_type files {};


	bool has_up = false;
	WIN32_FIND_DATAW up;

	for (auto &f : *v)
	{
		if (std::wstring { f.cFileName } == L"..")
		{
			// `..` directory is always first regardless of sort mode
			has_up = true;
			up = f;

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
		sort_mode == FileList::SORT_MODE_REVERSE_NAME ? [] (WIN32_FIND_DATAW & a, WIN32_FIND_DATAW & b) { return std::wstring { a.cFileName } > std::wstring { b.cFileName }; } :
		// size 
		sort_mode == FileList::SORT_MODE_SIZE ? [] (WIN32_FIND_DATAW &a, WIN32_FIND_DATAW &b) {
			return a.nFileSizeHigh < b.nFileSizeHigh || a.nFileSizeHigh == b.nFileSizeHigh && a.nFileSizeLow < b.nFileSizeLow;
		} :
		// reverse size
		sort_mode == FileList::SORT_MODE_REVERSE_SIZE ? [] (WIN32_FIND_DATAW &a, WIN32_FIND_DATAW &b) {
			return a.nFileSizeHigh > b.nFileSizeHigh || a.nFileSizeHigh == b.nFileSizeHigh && a.nFileSizeLow > b.nFileSizeLow;
		} :
		// default: NAME
		[] (WIN32_FIND_DATAW &a, WIN32_FIND_DATAW &b) { return std::wstring { a.cFileName } < std::wstring { b.cFileName }; }
	;

	std::sort (dirs.begin (), dirs.end (), sorter);
	std::sort (files.begin (), files.end (), sorter);

	v->clear ();
	// Combine .., other directories, and files
	v->reserve ((has_up ? 1 : 0) + dirs.size () + files.size ());
	if (has_up)
	{
		v->push_back (up);
	}
	std::ranges::copy (dirs, std::back_inserter (*v));
	std::ranges::copy (files, std::back_inserter (*v));

	return v;
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
	auto sort_mode = current_context->get<int> ("sort-mode", [] () { return new int { 0 }; });
	auto lst = current_context->get<File_list_type> ("list", [&] () {return list_directory_contents (*s); });
	auto sorted_lst = current_context->get<File_list_type> ("sorted-list", [&] () {return sort_file_list (lst, *sort_mode); });
	auto selected_file_idx = get_selected_file_idx (current_context);

	*selected_file_idx = min ((int) sorted_lst->size () - 1, *selected_file_idx);
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

	for (auto &fd : *sorted_lst)
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


	if (p < p_begin)
	{
		// The column is empty, so start from the beginning
		p = p_begin;
	}

	// Clear the remains
	while (p - p_begin < current_client_area.height)
	{
		fn_rect.y = current_client_area.y + p - p_begin;
		current_display->print_line
		(
			fn_rect,
			L" ",
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
	auto lst = current_context->get<File_list_type> ("sorted-list");

	if (modifiers & 1) // control
	{
		if (key == VK_F3) // Name
		{
			auto sort_mode = current_context->get<int> ("sort-mode");

			if (*sort_mode == FileList::SORT_MODE_NAME || *sort_mode == FileList::SORT_MODE_REVERSE_NAME)
			{
				*sort_mode ^= 1;
				current_context->erase ("sorted-list");
			}
		}
	}
	else if (key == VK_DOWN)
	{
		(*idx)++;
	}
	else if (key == VK_UP)
	{
		*idx = max (0, (*idx) - 1);
	}
	else if (key == VK_RIGHT)
	{
		(*idx) += current_client_area.height;
	}
	else if (key == VK_LEFT)
	{
		*idx = max (0, (*idx) - current_client_area.height);
	}
	else if (key == VK_RETURN)
	{
		if ((*lst)[*idx].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// change current directory
			auto pwd = current_context->get<std::wstring> ("pwd");
			auto next = std::wstring { (*lst)[*idx].cFileName };

			if (next == L"..")
			{
				// go up
				*pwd += L"\\..";
				wchar_t buf[2000];
				PathCanonicalize (buf, pwd->c_str ());
				*pwd = buf;
			}
			else
			{
				// go down
				*pwd += L"\\" + next;

			}
			*idx = 0;
			current_context->erase ("list");
			current_context->erase ("sorted-list");
		}
	}
	else if (key == VK_TAB)
	{
		// Switch panel focus
	}

	redraw_all ();
}

void FileList::redraw_all ()
{
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

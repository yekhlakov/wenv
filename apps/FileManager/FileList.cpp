#include <algorithm>
#include <map>
#include <regex>
#include <Shlwapi.h>
#include <Windows.h>
#include "../../maxy/strings.h"
#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../../display/Window.h"
#include "FileList.h"
#include "../../Context.h"

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

int *get_selected_file_idx (::Wenv::Context * c, const std::wstring &dirname)
{
	return c->get<int> ("selected-file-idx " + maxy::strings::wchartoutf8 (dirname), [] () ->int *{ return new int { 0 }; });
}

bool is_executable_file (const std::wstring &filename)
{
	static const std::wregex pattern (
		LR"(\.(exe|bat|com|cmd|ps1|msi)$)",
		std::regex_constants::icase
	);

	return std::regex_search (filename, pattern);
}

std::string FileList::get_sort_mode_name (int sort_mode)
{
	// Dictionary of the sort mode indication strings
	static const std::map<int, std::string> sort_mode_names
	{
		{ SORT_MODE_NAME,			"(↑Name)" },
		{ SORT_MODE_REVERSE_NAME,	"(↓Name)" },
		{ SORT_MODE_SIZE,			"(↑Size)" },
		{ SORT_MODE_REVERSE_SIZE,	"(↓Size)" },
		{ SORT_MODE_DATE,			"(↑Date)" },
		{ SORT_MODE_REVERSE_DATE,	"(↓Date)" }
	};

	auto it = sort_mode_names.find (sort_mode);

	return it != sort_mode_names.end () ? it->second : std::string {};
}

void FileList::draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area)
{
	App::draw (display, path, client_area);

	if (current_context == nullptr)
	{
		return;
	}

	redraw (path);
}

void FileList::redraw(const std::string &path)
{
	auto s = current_context->get<std::wstring> ("pwd");
	auto sort_mode = current_context->get<int> ("sort-mode", [] () { return new int { 0 }; });
	auto lst = current_context->get<File_list_type> ("list", [&] () {return list_directory_contents (*s); });
	auto sorted_lst = current_context->get<File_list_type> ("sorted-list", [&] () {return sort_file_list (lst, *sort_mode); });
	auto selected_file_idx = get_selected_file_idx (current_context, *s);
	auto current_client_area = get_client_area (path);
	auto list_offset = current_context->get<int> ("list-offset", [] () { return new int { 0 }; });

	*selected_file_idx = min ((int) sorted_lst->size () - 1, *selected_file_idx);
	*selected_file_idx = max (0, *selected_file_idx);

	// The top line is reserved for the sort mode indication. The remaining
	// rows hold the file entries, so the list size is one row shorter
	auto list_size = current_client_area.height - 1;

	if (*selected_file_idx < *list_offset)
	{
		// The cursor moved outside of viewable area, so shift it
		*list_offset = *selected_file_idx;
	}
	else if (*selected_file_idx >= *list_offset + 2 * list_size)
	{
		*list_offset = *selected_file_idx - 2 * list_size + 1;
	}

	// The sort mode indication is printed centered and highlighted only in the
	// leftmost column; the top line in the other columns is kept empty so all
	// the file lists have the same height
	::Wenv::Display::Rect sort_rect = current_client_area;
	sort_rect.height = 1;

	if (name[0] == L'2')
	{
		// Non-leftmost column: just an empty top line
		current_display->print_line
		(
			sort_rect,
			L" ",
			current_display->PF_LEFT | current_display->PF_ERASE_BACKGROUND
		);
	}
	else
	{
		// Active (regular) color for the sort mode indication
		current_display->with_color (::Wenv::Display::Palette::Active_element_color);
		current_display->print_line
		(
			sort_rect,
			maxy::strings::utf8towchar (get_sort_mode_name (*sort_mode)),
			current_display->PF_CENTER | current_display->PF_ERASE_BACKGROUND
		);
	}

	::Wenv::Display::Rect fn_rect = current_client_area;
	fn_rect.height = 1;

	int p = 0;
	int p_begin = *list_offset;
	if (name[0] == L'2')
	{
		// This is the right column so we must skip some of the first elements
		p_begin += list_size;
	}

	for (auto &fd : *sorted_lst)
	{
		if (p < p_begin)
		{
			p++;
			continue;
		}

		// Determine display color based on file attributes
		const char *color = ::Wenv::Display::Palette::Default_color;
		std::wstring display_name { fd.cFileName };

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
		{
			color = ::Wenv::Display::Palette::Dark_element_color;
		}
		else if (is_executable_file (display_name))
		{
			color = ::Wenv::Display::Palette::Active_element_color;
			display_name += L"*";
		}

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			display_name += L"/";
		}

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
		{
			display_name += L"&";
		}

		if (p == *selected_file_idx && current_display->focused_context == current_context)
		{
			current_display->with_color (color, true);
		}
		else
		{
			current_display->with_color (color);
		}

		// Skip the top line reserved for the sort mode indication
		fn_rect.y = current_client_area.y + 1 + p - p_begin;

		if (fn_rect.y - current_client_area.y >= current_client_area.height)
		{
			break;
		}

		current_display->print_line
		(
			fn_rect,
			display_name,
			current_display->PF_TOP | current_display->PF_LEFT | current_display->PF_ERASE_BACKGROUND
		);

		p++;
	}


	if (p < p_begin)
	{
		// The column is empty, so start from the beginning
		p = p_begin;
	}

	current_display->with_color (::Wenv::Display::Palette::Default_color);

	// Clear the remains of the skipped top line and the list area
	while (p - p_begin < current_client_area.height - 1)
	{
		fn_rect.y = current_client_area.y + 1 + p - p_begin;
		current_display->print_line
		(
			fn_rect,
			L" ",
			current_display->PF_TOP | current_display->PF_LEFT | current_display->PF_ERASE_BACKGROUND
		);

		p++;
	}
}

bool FileList::click (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers)
{
	// Focus this panel so the selection highlight and keypresses follow the click.
	// The focused app is a long-lived shared object, so it is stored without
	// ownership (Context::put) - Context::set would delete the previous app
	current_display->focused_app = this;
	current_display->focused_context = current_context;
	current_context->put ("focused-app", this);

	auto pwd = current_context->get<std::wstring> ("pwd");
	auto sort_mode = current_context->get<int> ("sort-mode", [] () { return new int { 0 }; });
	auto sorted_lst = current_context->get<File_list_type> ("sorted-list");
	auto idx = get_selected_file_idx (current_context, *pwd);
	auto list_offset = current_context->get<int> ("list-offset", [] () { return new int { 0 }; });
	auto list_size = client_area.height - 1;

	if (position.y == 0 && name[0] != L'2')
	{
		// The sort-mode indication is drawn on the first row of the leftmost
		// column; toggling it flips between ascending and descending
		*sort_mode ^= 1;
		current_context->erase ("sorted-list");
	}
	else if (position.y >= 1 && position.y - 1 < list_size)
	{
		// Compute the visible slice of the sorted list: the left panel starts
		// at list_offset, the right panel one list_size further down
		auto p_begin = *list_offset;
		if (name[0] == L'2')
		{
			p_begin += list_size;
		}

		auto file_idx = p_begin + position.y - 1;

		if (sorted_lst != nullptr && file_idx < (int) sorted_lst->size ())
		{
			*idx = file_idx;
		}
	}

	// The display redraws the whole screen when a click is handled
	return true;
}

void FileList::keypress (unsigned int key, int modifiers)
{
	auto pwd = current_context->get<std::wstring> ("pwd");
	auto idx = get_selected_file_idx (current_context, *pwd);
	auto lst = current_context->get<File_list_type> ("sorted-list");
	auto path = *current_context->get<std::string> ("focused-path");

	// The top line of the client area shows the sort indication
	auto list_size = client_areas[path].height - 1;

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
		(*idx) += list_size;
	}
	else if (key == VK_LEFT)
	{
		*idx = max (0, (*idx) - list_size);
	}
	else if (key == VK_RETURN)
	{
		if ((*lst)[*idx].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// change current directory
			auto next = std::wstring { (*lst)[*idx].cFileName };

			if (next == L"..")
			{
				// Forget the selected file of the directory we are leaving,
				// so when we come back we start from zero
				current_context->erase ("selected-file-idx " + maxy::strings::wchartoutf8 (*pwd));

				// go up
				*pwd += L"\\..";
				wchar_t buf[2000];
				PathCanonicalize (buf, pwd->c_str ());
				*pwd = buf;
			}
			else
			{
				// go down
				if (pwd->back () != L'\\')
				{
					*pwd += L"\\";
				}
				*pwd += next;

			}
			current_context->erase ("list");
			current_context->erase ("sorted-list");
		}
	}
	else if (key == VK_TAB)
	{
		// Switch panel focus
		auto n = current_context->get_name ();
		std::string target_context { "file-manager-left-panel" };
		if (n == target_context)
		{
			target_context = "file-manager-right-panel";
		}

		current_display->focused_context = nullptr;
		redraw_all (path);
		current_display->focused_context = current_display->get_context (target_context);
		with_context (current_display->focused_context);
		redraw_all (*current_display->focused_context->get<std::string>("focused-path"));
		return;
	}
	else if (key == VK_F3 || key == VK_F4)
	{
		// Show editor/viewer if this is not a directory
		if (!((*lst)[*idx].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			auto ed = current_display->window->get_display ("file-editor");
			auto ctx = ed->get_context ("file-editor");
			auto edit_mode = ctx->get<int> ("edit-mode", [] () {return new int {}; });
			*edit_mode = key == VK_F4 ? 1 : 0;
			auto edit_target = ctx->get<std::wstring> ("edit-target", [] () { return new std::wstring {}; });
			*edit_target = (*lst)[*idx].cFileName;
			auto edit_pwd = ctx->get<std::wstring> ("edit-pwd", [] () { return new std::wstring {}; });
			*edit_pwd = *current_context->get<std::wstring> ("pwd");

			current_display->window->set_display ("file-editor");
			return;
		}
	}

	redraw_all (path);
}

void FileList::redraw_all (const std::string & path)
{
	auto apps = current_context->get<std::vector<App *>> ("app-group");
	if (apps != nullptr)
	{
		for (auto app : *apps)
		{
			app->with_context(current_context)->redraw (path);
		}
	}
}

}

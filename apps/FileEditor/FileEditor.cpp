#include <Windows.h>
#include <string>
#include <vector>
#include "../../maxy/strings.h"
#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../Context.h"
#include "FileEditor.h"

namespace Wenv::Apps
{

// Load the whole file splitting it into lines (no line wrapping)
static std::wstring expand_tabs (const std::wstring &line)
{
	std::wstring out;
	int col = 0;

	for (auto ch : line)
	{
		if (ch == L'\t')
		{
			int spaces = 4 - (col % 4);
			out.append (spaces, L' ');
			col += spaces;
		}
		else
		{
			out += ch;
			col++;
		}
	}

	return out;
}

static bool load_file_lines (const std::wstring &path, std::vector<std::wstring> *lines)
{
	auto h = CreateFileW (path.c_str (), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (h == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	LARGE_INTEGER size;

	if (!GetFileSizeEx (h, &size))
	{
		CloseHandle (h);
		return false;
	}

	std::string raw;
	raw.resize ((size_t) size.QuadPart);

	DWORD read { 0 };
	auto ok = ReadFile (h, raw.data (), (DWORD) raw.size (), &read, nullptr);
	CloseHandle (h);

	if (!ok)
	{
		return false;
	}

	raw.resize (read);

	// Skip the utf8 BOM if it is there
	if (raw.size () >= 3 && (unsigned char) raw[0] == 0xEF && (unsigned char) raw[1] == 0xBB && (unsigned char) raw[2] == 0xBF)
	{
		raw.erase (0, 3);
	}

	std::string cur;

	for (auto c : raw)
	{
		if (c == '\n')
		{
			lines->push_back (maxy::strings::utf8towchar (cur));
			cur.clear ();
		}
		else if (c != '\r')
		{
			cur += c;
		}
	}

	// The last line may have no trailing newline
	lines->push_back (maxy::strings::utf8towchar (cur));

	return true;
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
	auto target = current_context->get<std::wstring> ("edit-target");
	auto pwd = current_context->get<std::wstring> ("edit-pwd");

	if (target == nullptr || target->empty () || pwd == nullptr || pwd->empty ())
	{
		return;
	}

	auto full_path = *pwd;

	if (full_path.back () != L'\\' && full_path.back () != L'/')
	{
		full_path += L"\\";
	}

	full_path += *target;

	auto content = current_context->get<std::vector<std::wstring>> ("content");
	auto viewed_path = current_context->get<std::wstring> ("viewed-path");

	if (content == nullptr || viewed_path == nullptr || *viewed_path != full_path)
	{
		// A different file was requested - forget the old content and offsets
		content = new std::vector<std::wstring> {};
		load_file_lines (full_path, content);

		current_context->set ("content", content);
		current_context->set ("viewed-path", new std::wstring { full_path });

		current_context->erase ("top-line");
		current_context->erase ("left-col");
	}

	auto area = get_client_area (path);
	auto top = current_context->get<int> ("top-line", [] () { return new int {}; });
	auto left = current_context->get<int> ("left-col", [] () { return new int {}; });

	size_t longest { 0 };
	for (auto &l : *content)
	{
		longest = max (longest, expand_tabs (l).size ());
	}

	// Clamp the visible window to the content
	*top = min (*top, max (0, (int) content->size () - area.height));
	*top = max (*top, 0);
	*left = min (*left, max (0, (int) longest - area.width));
	*left = max (*left, 0);

	current_display->with_color (::Wenv::Display::Palette::Default_color);

	for (int row = 0; row < area.height; row++)
	{
		auto ln = *top + row;

		::Wenv::Display::Rect r = area;
		r.y += row;
		r.height = 1;

		auto s = ln < (int) content->size ()
			? expand_tabs ((*content)[ln]).substr ((size_t) *left)
			: std::wstring { L" " };

		if (s.empty ())
		{
			s = L" ";
		}

		current_display->print_line
		(
			r,
			s,
			current_display->PF_TOP | current_display->PF_LEFT | current_display->PF_ERASE_BACKGROUND
		);
	}
}

void FileEditor::click (::Wenv::Display::Rect client_area, int modifiers)
{
}

void FileEditor::keypress (unsigned int key, int modifiers)
{
	if (current_context == nullptr)
	{
		return;
	}

	auto path = *current_context->get<std::string> ("focused-path");
	auto area = get_client_area (path);
	auto top = current_context->get<int> ("top-line", [] () { return new int {}; });
	auto left = current_context->get<int> ("left-col", [] () { return new int {}; });

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
		return;
	}

	// The offsets are clamped to the content bounds during redraw
	redraw_all (path);
}

void FileEditor::redraw_all (const std::string &path)
{
	auto apps = current_context->get<std::vector<App *>> ("app-group");

	if (apps != nullptr)
	{
		for (auto app : *apps)
		{
			app->with_context (current_context)->redraw (path);
		}
	}
}

}

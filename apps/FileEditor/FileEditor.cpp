#include <Windows.h>
#include <iterator>
#include <string>
#include <vector>
#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../Context.h"
#include "File.h"
#include "FileEditor.h"

namespace Wenv::Apps
{

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

	auto file = current_context->get<File> ("file");
	auto viewed_path = current_context->get<std::wstring> ("viewed-path");

	if (file == nullptr || viewed_path == nullptr || *viewed_path != full_path)
	{
		// A different file was requested - load it
		file = new File { full_path };

		current_context->set ("file", file);
		current_context->set ("viewed-path", new std::wstring { full_path });

		current_context->erase ("top-line");
		current_context->erase ("left-col");
	}

	auto area = get_client_area (path);
	auto top = current_context->get<int> ("top-line", [] () { return new int {}; });
	auto left = current_context->get<int> ("left-col", [] () { return new int {}; });

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
		std::vector<int> tab_positions;
		if (ln < (int) file->lines.size ())
		{
			auto result = expand_tabs (it->raw_data);
			expanded = std::move (result.first);
			tab_positions = std::move (result.second);
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

		// Draw tab markers in a separate pass using stored positions
		if (tab_positions.size() > 0)
		{
			current_display->with_color (::Wenv::Display::Palette::Dark_element_color);
			for (auto pos : tab_positions)
			{
				int screen_x = pos - *left;
				if (screen_x >= 0 && screen_x < area.width)
				{
					current_display->print_char (area.x + screen_x, r.y, L'\u2192');
				}
			}
		}

		// Colorize the truncation ellipsis with the active palette color
		if (truncated)
		{
			current_display->with_color (::Wenv::Display::Palette::Active_element_color);
			current_display->print_char (area.x + area.width - 1, r.y, L'\u2026');
		}

		if (tab_positions.size() > 0 || truncated)
		{
			// Return the default color if we've changed it to some other color previously
			current_display->with_color (::Wenv::Display::Palette::Default_color);
		}
	}

	// Update the status bar
	auto status = current_context->get<::Wenv::Apps::App> ("status-bar");
	if (status != nullptr)
	{
		status->with_context (current_context)->redraw (path);
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

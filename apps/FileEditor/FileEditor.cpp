#include <Windows.h>
#include <iterator>
#include <string>
#include <vector>
#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../../maxy/strings.h"
#include "../../Context.h"
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
	auto target = get_edit_target ();
	auto pwd = get_edit_pwd ();

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

	auto file = get_file ();
	auto viewed_path = get_viewed_path ();

	if (file == nullptr || viewed_path == nullptr || *viewed_path != full_path)
	{
		// A different file was requested - load it
		file = new File { full_path };

		current_context->set ("file", file);
		current_context->set ("viewed-path", new std::wstring { full_path });
	}

	auto area = get_client_area (path);

	// Store position in persistent context per file, keyed by full path
	auto top = get_file_top_line (full_path);
	auto left = get_file_left_column (full_path);

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
		std::vector<std::pair<int, int>> tab_spans;
		if (ln < (int) file->lines.size ())
		{
			auto result = expand_tabs (it->raw_data);
			expanded = std::move (result.first);
			tab_spans = std::move (result.second);
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

		bool drew_dark = false;

		// Draw trailing spaces as middle dots
		if (!expanded.empty ())
		{
			auto last_non_space = expanded.find_last_not_of (L' ');
			if (last_non_space != std::wstring::npos)
			{
				int trailing_start = (int) last_non_space + 1;
				int trailing_count = (int) expanded.size () - trailing_start;

				if (trailing_count > 0)
				{
					current_display->with_color (::Wenv::Display::Palette::Dark_element_color);
					drew_dark = true;

					for (int i = 0; i < trailing_count; i++)
					{
						int screen_x = trailing_start + i - *left;
						if (screen_x >= 0 && screen_x < area.width)
						{
							current_display->print_char (area.x + screen_x, r.y, L'\u00B7');
						}
					}
				}
			}
		}

		// Draw tab markers in a separate pass using stored spans
		if (tab_spans.size() > 0)
		{
			if (!drew_dark)
			{
				current_display->with_color (::Wenv::Display::Palette::Dark_element_color);
				drew_dark = true;
			}
			for (auto &[pos, len] : tab_spans)
			{
				int screen_x = pos - *left;
				int vis_start = max (0, screen_x);
				int vis_end = min (area.width, screen_x + len);
				int vis_width = vis_end - vis_start;

				if (vis_width > 0)
				{
					::Wenv::Display::Rect tab_rect;
					tab_rect.x = area.x + vis_start;
					tab_rect.y = r.y;
					tab_rect.width = vis_width;
					tab_rect.height = 1;

					current_display->print_line
					(
						tab_rect,
						L"\u2192",
						current_display->PF_RIGHT | current_display->PF_VCENTER | current_display->PF_ERASE_BACKGROUND
					);
				}
			}
		}

		// Colorize the truncation ellipsis with the active palette color
		if (truncated)
		{
			current_display->with_color (::Wenv::Display::Palette::Active_element_color);
			current_display->print_char (area.x + area.width - 1, r.y, L'\u2026');
		}

		if (drew_dark || truncated)
		{
			// Return the default color if we've changed it to some other color previously
			current_display->with_color (::Wenv::Display::Palette::Default_color);
		}
	}

	// Update the status bar
	auto status = get_status_bar ();
	if (status != nullptr)
	{
		status->with_context (current_context)->redraw (path);
	}
}

bool FileEditor::handle_click (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers)
{
	return false;
}

bool FileEditor::handle_keydown (unsigned int key, int modifiers)
{
	if (current_context == nullptr)
	{
		return false;
	}

	auto path = *get_focused_path ();
	auto area = get_client_area (path);

	// Reconstruct full path to look up per-file position in persistent context
	auto target = get_edit_target ();
	auto pwd = get_edit_pwd ();
	if (target == nullptr || pwd == nullptr)
	{
		return false;
	}

	auto full_path = *pwd;
	if (full_path.back () != L'\\' && full_path.back () != L'/')
	{
		full_path += L"\\";
	}
	full_path += *target;

	auto top = get_file_top_line (full_path);
	auto left = get_file_left_column (full_path);

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
		return false;
	}

	// The offsets are clamped to the content bounds during redraw
	redraw_all (path);

	return true;
}

void FileEditor::redraw_all (const std::string &path)
{
	auto apps = get_app_group ();

	if (apps != nullptr)
	{
		for (auto app : *apps)
		{
			app->with_context (current_context)->redraw (path);
		}
	}
}

std::wstring * FileEditor::get_edit_target ()
{
	// No default can be provided for this parameter because it is set
	// from outside (the file manager) when the editor is opened
	return current_context->get<std::wstring> ("edit-target");
}

std::wstring * FileEditor::get_edit_pwd ()
{
	// No default can be provided for this parameter because it is set
	// from outside (the file manager) together with edit-target
	return current_context->get<std::wstring> ("edit-pwd");
}

std::wstring * FileEditor::get_viewed_path ()
{
	// No default: a missing value means no file has been viewed yet, which
	// makes the first redraw load the requested file
	return current_context->get<std::wstring> ("viewed-path");
}

File * FileEditor::get_file ()
{
	// No default: the file is loaded and stored by the editor itself
	return current_context->get<File> ("file");
}

App * FileEditor::get_status_bar ()
{
	// No default: an absent status bar simply leaves nothing to refresh
	return current_context->get<App> ("status-bar");
}

std::string * FileEditor::get_focused_path ()
{
	// Set by the core when the display is built; no default makes sense here
	return current_context->get<std::string> ("focused-path");
}

std::vector<App *> * FileEditor::get_app_group ()
{
	// No default: a missing group simply means there are no apps to redraw
	return current_context->get<std::vector<App *>> ("app-group");
}

int * FileEditor::get_file_top_line (const std::wstring &full_path)
{
	// The per-file viewport is kept in the persistent context under a key
	// derived from the file path; a freshly opened file starts at the top
	auto per_file_key = std::string { "top-line:" } + maxy::strings::wchartoutf8 (full_path);

	return current_display->get_persistent_context ()->get<int> (per_file_key, [] () { return new int { 0 }; });
}

int * FileEditor::get_file_left_column (const std::wstring &full_path)
{
	// A freshly opened file starts at the leftmost column
	auto per_file_key = std::string { "left-col:" } + maxy::strings::wchartoutf8 (full_path);

	return current_display->get_persistent_context ()->get<int> (per_file_key, [] () { return new int { 0 }; });
}

}

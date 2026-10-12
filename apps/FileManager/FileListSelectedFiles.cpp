#include <Windows.h>
#include <format>
#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../../Context.h"
#include "FileList.h"
#include "FileListSelectedFiles.h"

namespace Wenv::Apps
{

void FileListSelectedFiles::draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area)
{
	App::draw (display, path, client_area);
	redraw (path);
}

void FileListSelectedFiles::redraw (const std::string &path)
{
	auto ca = get_client_area (path);

	if (current_context == nullptr)
	{
		return;
	}

	// The selection and its totals are maintained by the file list of the
	// same panel in the shared context
	auto selected = current_context->get<std::vector<std::wstring>> ("selected-files");
	auto stats = current_context->get<Selected_stats> ("selected-stats");

	// Restore the boundary crossing the row first: it wipes the remains of
	// the previous contents and keeps the boundary visible under the strip
	current_display->redraw_boundaries_on_row (ca.y, ca.x, ca.x + ca.width);

	// Nothing is shown while nothing is selected
	if (selected == nullptr || stats == nullptr || selected->empty ())
	{
		return;
	}

	// The summary of the selection, printed over the boundary
	current_display->with_color (::Wenv::Display::Palette::Active_element_color);
	current_display->print_line
	(
		ca,
		std::format (L"Selected: {} files, {} dirs, {} bytes", stats->files, stats->dirs, stats->bytes),
		current_display->PF_CENTER
	);
}

}

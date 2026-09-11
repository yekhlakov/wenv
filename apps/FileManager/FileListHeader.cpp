#include <Windows.h>
#include "../../display/Display.h"
#include "../../Context.h"
#include "FileListHeader.h"

namespace Wenv::Apps
{

void FileListHeader::draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area)
{
	App::draw (display, path, client_area);
	redraw (path);
}

void FileListHeader::redraw (const std::string &path)
{
	auto pwd = current_context->get<std::wstring> ("pwd");
	auto ca = get_client_area (path);

	// The directory name is drawn across the box boundary of the panel, so restore
	// the boundary first: this both erases the remnants of the previous name and
	// brings the border line back
	current_display->redraw_boundaries_on_row (ca.y, ca.x, ca.x + ca.width);

	current_display->print_line
	(
		ca,
		L" " + *pwd + L" ",
		current_display->PF_CENTER
	);
}

}
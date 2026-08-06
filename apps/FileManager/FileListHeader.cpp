#include <Windows.h>
#include "../../display/Display.h"
#include "../Context.h"
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
	current_display->print_line
	(
		get_client_area(path),
		L" " + *pwd + L" ",
		current_display->PF_CENTER
	);
}

}
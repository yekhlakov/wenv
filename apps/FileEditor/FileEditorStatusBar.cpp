#include "../../display/Display.h"
#include "FileEditorStatusBar.h"

namespace Wenv::Apps
{
	void FileEditorStatusBar::draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area)
	{
		display.print_line
		(
			client_area,
			L"----<==<{   Editor Status   }>==>----",
			display.PF_CENTER | display.PF_ERASE_BACKGROUND
		);
	}

	void FileEditorStatusBar::redraw (const std::string &path)
	{
		// Dummy implementation for redrawing status bar
	}
}
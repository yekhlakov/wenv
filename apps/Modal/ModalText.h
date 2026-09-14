#pragma once

#include <string>
#include "../App.h"

namespace Wenv::Apps
{

// The app that draws the body text in the middle block of a modal box.
// It reads the text string and the text color from the "modal" context,
// wraps the text to fit the client area, and draws the wrapped lines.
class ModalText : public App
{
public:
	ModalText (const std::wstring &n) : App { n } {}

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
};

} // namespace Wenv::Apps

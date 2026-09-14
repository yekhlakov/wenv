#pragma once

#include <string>
#include "../App.h"

namespace Wenv::Apps
{

// The app that draws the button bar in the bottom block of a modal box.
// It reads the button list and the border color from the "modal" context
// and draws each button as "[ TEXT ]", centered horizontally.
class ModalButtons : public App
{
public:
	ModalButtons (const std::wstring &n) : App { n } {}

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
};

} // namespace Wenv::Apps

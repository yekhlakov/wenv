#pragma once

#include <string>
#include "../App.h"

namespace Wenv::Apps
{

// The app that draws the title in the top block of a modal box.
// It reads the title string and the title color from the "modal" context.
class ModalTitle : public App
{
public:
	ModalTitle (const std::wstring &n) : App { n } {}

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
};

} // namespace Wenv::Apps

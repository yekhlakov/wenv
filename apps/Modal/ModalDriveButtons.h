#pragma once

#include "ModalButtons.h"

namespace Wenv::Apps
{

// The app that draws the extra button row of a modal box (e.g. the drive
// buttons of the "go to directory" modal). It renders and clicks exactly
// like the bottom button bar, but reads its buttons from the "modal-extra-buttons"
// element of the "modal" context. The keyboard focus of the modal is moved
// over all of its buttons by the tab key: the extra row buttons first, then
// the buttons of the bottom row, cyclically; the enter activates the focused
// extra row button (or falls through to the bottom row when the focus sits
// there). The remaining keys (the arrows, the typed characters) belong to
// the text input and are left alone
class ModalDriveButtons : public ModalButtons
{
	// The buttons and the active button index are read from the dedicated
	// context elements (see the .cpp)
	std::vector<::Wenv::Display::ModalButton> * get_buttons () override;
	int * get_active_button () override;

public:
	ModalDriveButtons (const std::wstring &n) : ModalButtons { n } {}

	// Moves the keyboard focus along the buttons of the whole modal (tab)
	// and activates the focused button of this row (enter)
	virtual bool handle_keydown (unsigned int key, int modifiers) override;
};

} // namespace Wenv::Apps

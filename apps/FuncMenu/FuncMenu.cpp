#include <Windows.h>
#include <format>
#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../../Context.h"
#include "FuncMenu.h"

namespace Wenv::Apps
{

std::array<FuncMenuCommand, 12> &FuncMenu::get_command_list (FuncMenuCommandList list)
{
	switch (list)
	{
	case FuncMenuCommandList::Ctrl: return ctrl_commands;
	case FuncMenuCommandList::Alt: return alt_commands;
	case FuncMenuCommandList::Shift: return shift_commands;
	case FuncMenuCommandList::CtrlShift: return ctrl_shift_commands;
	case FuncMenuCommandList::CtrlAlt: return ctrl_alt_commands;
	default: return commands;
	}
}

void FuncMenu::set_command (int number, const std::wstring &name, FuncMenuAction action, FuncMenuCommandList list)
{
	if (number < 1 || number > 12)
	{
		// The func menu only has slots for the F1..F12 keys
		return;
	}

	get_command_list (list)[number - 1] = FuncMenuCommand { name, std::move (action) };
}

void FuncMenu::erase_command (int number, FuncMenuCommandList list)
{
	if (number < 1 || number > 12)
	{
		return;
	}

	get_command_list (list)[number - 1] = FuncMenuCommand {};
}

FuncMenuCommandList FuncMenu::get_active_command_list () const
{
	if (current_display == nullptr)
	{
		return FuncMenuCommandList::Default;
	}

	// The pressed modifiers decide which command list is shown.
	// The combinations without their own lists (alt+shift, ctrl+alt+shift)
	// fall back to the closest list above them.
	auto ctrl = current_display->get_key_state (VK_CONTROL);
	auto alt = current_display->get_key_state (VK_MENU);
	auto shift = current_display->get_key_state (VK_SHIFT);

	if (ctrl && alt)
	{
		return FuncMenuCommandList::CtrlAlt;
	}

	if (ctrl && shift)
	{
		return FuncMenuCommandList::CtrlShift;
	}

	if (ctrl)
	{
		return FuncMenuCommandList::Ctrl;
	}

	if (alt)
	{
		return FuncMenuCommandList::Alt;
	}

	if (shift)
	{
		return FuncMenuCommandList::Shift;
	}

	return FuncMenuCommandList::Default;
}

void FuncMenu::draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area)
{
	App::draw (display, path, client_area);
	own_area = client_area;
	redraw (path);
}

void FuncMenu::redraw (const std::string &path)
{
	// The bar drawing only depends on the display; the context may be
	// absent when the menu is attached to a context-less block
	if (current_display == nullptr || own_area.width < 1)
	{
		return;
	}

	// Clear the bar so the remains of previous contents do not show through
	current_display->with_color (::Wenv::Display::Palette::Default_color);
	current_display->print_line (own_area, L"", current_display->PF_ERASE_BACKGROUND);

	auto slot_width = own_area.width / 12;

	// Show the commands of the list matching the currently pressed modifiers
	auto &active_commands = get_command_list (get_active_command_list ());

	for (size_t i = 0; i < active_commands.size (); i++)
	{
		auto &command = active_commands[i];
		auto number_text = std::format (L"{}", i + 1);

		// The label is centered within the key slot
		auto label = command.name.empty () ? number_text : number_text + L" " + command.name;
		auto start_x = own_area.x + (int) i * slot_width + (slot_width - (int) label.size ()) / 2;

		// The function key number is drawn in the active color
		current_display->with_color (::Wenv::Display::Palette::Active_element_color);
		current_display->print_line (start_x, own_area.y, number_text);

		if (!command.name.empty ())
		{
			// The command name is drawn highlighted right after the number
			current_display->with_color (::Wenv::Display::Palette::Default_color, true);
			current_display->print_line (start_x + (int) number_text.size () + 1, own_area.y, command.name);
		}
	}

	// Restore the default color so later draws are not affected by the highlight
	current_display->with_color (::Wenv::Display::Palette::Default_color);
}

bool FuncMenu::click (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers)
{
	return false;
}

void FuncMenu::keypress (unsigned int key, int modifiers)
{
	if (current_display == nullptr || current_context == nullptr)
	{
		return;
	}

	// Run the command bound to the pressed function key, if any.
	// The command comes from the list of the currently pressed modifiers.
	auto &active_commands = get_command_list (get_active_command_list ());

	if (key >= VK_F1 && key <= VK_F12 && active_commands[key - VK_F1].action)
	{
		active_commands[key - VK_F1].action (current_display, current_context);
		redraw_all ();
		return;
	}

	// Then propagate to focused app
	auto focused_app = current_display->focused_context->get<::Wenv::Apps::App> ("focused-app");
	if (focused_app != nullptr)
	{
		focused_app->with_context (current_display->focused_context)->keypress (key, modifiers);
	}
}

void FuncMenu::redraw_all ()
{
	if (current_context == nullptr)
	{
		return;
	}

	auto apps = current_context->get<std::vector<App *>> ("app-group");
	if (apps == nullptr)
	{
		return;
	}

	// Redraw the apps using the path of the currently focused app
	auto focused_path = current_context->get<std::string> ("focused-path");
	auto path = focused_path != nullptr ? *focused_path : std::string {};

	for (auto app : *apps)
	{
		app->with_context (current_context)->redraw (path);
	}
}

}
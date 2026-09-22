#include <Windows.h>
#include <format>
#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../../Context.h"
#include "FuncMenu.h"

namespace Wenv::Apps
{

// The name of the current-context element that holds the command list of the
// given modifier combination
static const char * command_list_name (FuncMenuCommandList list)
{
	switch (list)
	{
	case FuncMenuCommandList::Ctrl: return "func_menu.ctrl";
	case FuncMenuCommandList::Alt: return "func_menu.alt";
	case FuncMenuCommandList::Shift: return "func_menu.shift";
	case FuncMenuCommandList::CtrlShift: return "func_menu.ctrl_shift";
	case FuncMenuCommandList::CtrlAlt: return "func_menu.ctrl_alt";
	default: return "func_menu.default";
	}
}

std::vector<FuncMenuCommand> &FuncMenu::get_list (FuncMenuCommandList list)
{
	// A missing context means the menu shows the bare function key numbers
	static std::vector<FuncMenuCommand> empty;

	if (current_context == nullptr)
	{
		return empty;
	}

	// The command list is read from the current context; a missing element
	// is an empty list
	return *current_context->get<std::vector<FuncMenuCommand>> (
		command_list_name (list),
		[] { return new std::vector<FuncMenuCommand> {}; }
	);
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

	// Show the commands of the list matching the currently pressed modifiers,
	// read from the current context
	auto &active_commands = get_list (get_active_command_list ());

	for (size_t i = 0; i < 12; i++)
	{
		auto number_text = std::format (L"{}", i + 1);
		auto has_command = i < active_commands.size () && !active_commands[i].name.empty ();

		// The label is centered within the key slot
		auto label = has_command ? number_text + L" " + active_commands[i].name : number_text;
		auto start_x = own_area.x + (int) i * slot_width + (slot_width - (int) label.size ()) / 2;

		// The function key number is drawn in the active color
		current_display->with_color (::Wenv::Display::Palette::Active_element_color);
		current_display->print_line (start_x, own_area.y, number_text);

		if (has_command)
		{
			// The command name is drawn highlighted right after the number
			current_display->with_color (::Wenv::Display::Palette::Default_color, true);
			current_display->print_line (start_x + (int) number_text.size () + 1, own_area.y, active_commands[i].name);
		}
	}

	// Restore the default color so later draws are not affected by the highlight
	current_display->with_color (::Wenv::Display::Palette::Default_color);
}

bool FuncMenu::handle_click (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers)
{
	return false;
}

bool FuncMenu::handle_keydown (unsigned int key, int modifiers)
{
	if (current_display == nullptr || current_context == nullptr)
	{
		return false;
	}

	// Run the command bound to the pressed function key, if any.
	// The command comes from the list of the currently pressed modifiers,
	// read from the current context.
	auto &active_commands = get_list (get_active_command_list ());

	if (key >= VK_F1 && key <= VK_F12)
	{
		auto index = key - VK_F1;

		if (index < active_commands.size () && active_commands[index].action)
		{
			active_commands[index].action (current_display, current_context);
			redraw_all ();
			return true;
		}
	}

	// Then propagate to focused app
	auto focused_app = current_display->focused_context->get<::Wenv::Apps::App> ("focused-app");
	if (focused_app != nullptr)
	{
		return focused_app->with_context (current_display->focused_context)->handle_keydown (key, modifiers);
	}

	return false;
}

bool FuncMenu::handle_keyup (unsigned int key, int modifiers)
{
	return false;
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
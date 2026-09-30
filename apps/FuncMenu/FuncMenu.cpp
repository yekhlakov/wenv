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

// Which modifier (if any) the given virtual key belongs to:
// 1 = ctrl, 2 = shift, 4 = alt; 0 when the key is not a modifier.
// The generic codes and the left/right-specific ones are matched alike,
// as the key messages carry the specific variants
static int modifier_bit (unsigned int key)
{
	switch (key)
	{
	case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL: return 1;
	case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT: return 2;
	case VK_MENU: case VK_LMENU: case VK_RMENU: return 4;
	default: return 0;
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
	// The tracked state of the modifier keys decides which command list is
	// shown. The combinations without their own lists (alt+shift,
	// ctrl+alt+shift) fall back to the closest list above them.
	if (tracked_ctrl && tracked_alt)
	{
		return FuncMenuCommandList::CtrlAlt;
	}

	if (tracked_ctrl && tracked_shift)
	{
		return FuncMenuCommandList::CtrlShift;
	}

	if (tracked_ctrl)
	{
		return FuncMenuCommandList::Ctrl;
	}

	if (tracked_alt)
	{
		return FuncMenuCommandList::Alt;
	}

	if (tracked_shift)
	{
		return FuncMenuCommandList::Shift;
	}

	return FuncMenuCommandList::Default;
}

// Update the tracked state of the modifier the given key belongs to and
// redraw the bar when the tracked combination changes; the auto-repeated
// presses of a held key do not change it
bool FuncMenu::track_modifier (unsigned int key, bool pressed)
{
	auto changed = false;

	if (modifier_bit (key) & 1)
	{
		changed = tracked_ctrl != pressed;
		tracked_ctrl = pressed;
	}
	else if (modifier_bit (key) & 2)
	{
		changed = tracked_shift != pressed;
		tracked_shift = pressed;
	}
	else if (modifier_bit (key) & 4)
	{
		changed = tracked_alt != pressed;
		tracked_alt = pressed;
	}

	if (changed)
	{
		redraw ("");
	}

	return true;
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
	if (current_display == nullptr || current_context == nullptr)
	{
		return false;
	}

	// The bar is split into twelve key slots of even width (as during the
	// drawing); the trailing space past the twelfth slot belongs to the last key
	auto slot_width = client_area.width / 12;

	if (slot_width < 1 || position.x < 0)
	{
		return false;
	}

	auto index = position.x / slot_width;

	if (index > 11)
	{
		index = 11;
	}

	// Only a command that is defined and currently displayed can be invoked
	auto &active_commands = get_list (get_active_command_list ());

	if (index >= (int) active_commands.size () || active_commands[index].name.empty () || !active_commands[index].action)
	{
		return false;
	}

	// The command runs exactly like the pressed key combo: the menu binds the
	// focused context, so the action and the following redraw observe the same
	// state the keydown handler would
	if (current_display->focused_context != nullptr)
	{
		with_context (current_display->focused_context);
	}

	active_commands[index].action (current_display, current_context);
	redraw_all ();

	return true;
}

bool FuncMenu::handle_keydown (unsigned int key, int modifiers)
{
	if (current_display == nullptr || current_context == nullptr)
	{
		return false;
	}

	// The modifier keypresses are consumed by the tracking and make the bar
	// display the command list of the new combination
	if (modifier_bit (key) != 0)
	{
		return track_modifier (key, true);
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

			// An action that has opened a modal leaves the display fully
			// redrawn with the modal on top; the apps must not redraw
			// themselves over it
			if (current_display->current_modal == nullptr)
			{
				redraw_all ();
			}

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
	if (current_display == nullptr || current_context == nullptr)
	{
		return false;
	}

	// The release of a modifier key also changes the tracked combination
	if (modifier_bit (key) != 0)
	{
		return track_modifier (key, false);
	}

	return false;
}

void FuncMenu::redraw_all ()
{
	if (current_context == nullptr)
	{
		return;
	}

	auto apps = get_app_group ();
	if (apps == nullptr)
	{
		return;
	}

	// Redraw the apps using the path of the currently focused app
	auto focused_path = get_focused_path ();
	auto path = focused_path != nullptr ? *focused_path : std::string {};

	for (auto app : *apps)
	{
		app->with_context (current_context)->redraw (path);
	}
}

std::vector<App *> * FuncMenu::get_app_group ()
{
	// No default: a missing group simply means there are no apps to redraw
	return current_context->get<std::vector<App *>> ("app-group");
}

std::string * FuncMenu::get_focused_path ()
{
	// Set by the core when the display is built; a missing value simply
	// leaves the redraw with an empty path
	return current_context->get<std::string> ("focused-path");
}

}
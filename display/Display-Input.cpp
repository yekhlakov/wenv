#include <format>
#include "Display.h"
#include "Modal.h"
#include "Window.h"
#include "../apps/App.h"
#include "../Context.h"
#include "../layout/Grid.h"

namespace Wenv::Display
{

// The current pressed state of the given key (proxied to the window)
bool Display::get_key_state (int key) const
{
	if (window == nullptr)
	{
		return false;
	}

	return window->get_key_state (key);
}

// Find the app the given point falls into within the given grid, recursing into
// nested grids. The point is in display (character) coordinates. Empty when the
// point hits no app: a border, an empty block or the space between blocks
Display::App_hit Display::find_app_at (::Wenv::Layout::Grid &grid, const std::string &path, int x, int y, ::Wenv::Context *ctx)
{
	// The default context for the blocks of the grid
	if (ctx == nullptr)
	{
		ctx = grid.context;
	}

	int bnum = 0;
	for (auto &b : grid.blocks)
	{
		auto bpath = std::format ("{}.{}", path, bnum++);

		auto instance = b.instances.find (bpath);
		if (instance == b.instances.end ())
		{
			// The grid has not been baked yet: its geometry is unknown
			continue;
		}

		auto &container = instance->second.container_dimensions;

		// Skip the blocks the point does not fall into at all
		if (x < container.x || x >= container.x + container.width ||
			y < container.y || y >= container.y + container.height)
		{
			continue;
		}

		// A block app is drawn on top of a nested grid, so it takes precedence
		if (b.app != nullptr)
		{
			auto area = b.get_client_dimensions (bpath);

			if (x >= area.x && x < area.x + area.width &&
				y >= area.y && y < area.y + area.height)
			{
				return { b.app, b.get_context (ctx), bpath, area };
			}
		}

		// Look for a deeper app inside the nested grid; an app must be a hit
		// even when its client area overlaps that of the parent block
		if (b.grid != nullptr)
		{
			auto hit = find_app_at (*b.grid, bpath, x, y, b.get_context (ctx));

			if (hit)
			{
				return hit;
			}
		}
	}

	return {};
}

// Handle a mouse click at the given character coordinates. When a modal is
// visible only the apps of the modal are examined, otherwise the apps of the
// current layout; a click on a border is ignored. Returns true when an app
// handled the click and the whole screen must be redrawn
bool Display::handle_mouse_click (int x, int y, int modifiers)
{
	App_hit hit;

	if (current_modal != nullptr)
	{
		// While a modal is visible only its apps react to the click, mirroring
		// the way keypresses are routed; a click outside the modal is ignored
		if (current_modal->grid == nullptr)
		{
			return false;
		}

		hit = find_app_at (*current_modal->grid, "modal", x, y, get_context ("modal"));
	}
	else
	{
		if (grid == nullptr)
		{
			return false;
		}

		hit = find_app_at (*grid, "root", x, y);
	}

	if (!hit)
	{
		return false;
	}

	// The position of the click within the app's client area
	Pos position { x - hit.client_area.x, y - hit.client_area.y };

	auto handled = hit.app->with_context (hit.context)->handle_click (hit.client_area, position, modifiers);

	// The click handler may have changed app state (selection, sort mode,
	// focus); the caller redraws the whole display when the click was handled
	return handled;
}

} // namespace Wenv::Display
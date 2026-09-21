#include <random>
#include "Display.h"
#include "Modal.h"
#include "Window.h"
#include "../apps/App.h"
#include "../Context.h"
#include "../layout/Grid.h"

namespace Wenv::Display
{

// Construct a named display with no grid yet (the grid is supplied by the
// layout during application initialization)
Display::Display (const std::wstring &n) :
	name { n }
{
}

// Destroy the display and release its grid
Display::~Display ()
{
	if (grid != nullptr)
	{
		delete grid;
	}
}

// Resize the display: regenerate the character buffer, bake the grid into it
// and redraw the contents; the current modal (if any) is drawn on top
void Display::resize (size_t width, size_t height)
{
	data.resize (height);
	std::mt19937 gen (std::random_device {}());
	std::uniform_int_distribution<int> ascii_dist (0x0020, 0x007E);
	std::uniform_int_distribution<int> cyrillic_dist (0x0400, 0x045F);
	std::uniform_int_distribution<int> region_dist (0, 1);
	std::uniform_int_distribution<int> palette_dist (0, 16);
	//std::uniform_int_distribution<int> color_dist(0, 0xFFFFFF);


	for (size_t y = 0; y < height; ++y) {
		data[y].resize (width);
		for (size_t x = 0; x < width; ++x) {
			wchar_t ch;
			//int region = region_dist(gen);
			//if (region == 0) 
			//ch = static_cast<wchar_t>(ascii_dist(gen));
			//else 
			ch = static_cast<wchar_t> (cyrillic_dist (gen));

			data[y][x].set (ch, palette_dist (gen));
		}
	}

	// Char buffer to hold boundary texts
	std::vector<std::vector<wchar_t>> buffer (height, std::vector<wchar_t> (width + 1, L' '));

	grid->bake ({ 0, 0, (int) width, (int) height }, buffer, "root");

	draw_grid (*grid, "root");

	// The current modal (if any) is drawn on top of the display contents
	if (current_modal != nullptr)
	{
		current_modal->draw (*this);
	}
}

// Register an app with the display; apps that want all keypresses are added
// to the "listening apps" list
::Wenv::Apps::App *Display::add_app (::Wenv::Apps::App *a)
{
	all_apps.push_back (a);
	if (a->wants_all_keypresses ())
	{
		listening_apps.push_back (a);
	}

	return a;
}

// Register a named context (replacing any previous one under the same name)
::Wenv::Context *Display::add_context (::Wenv::Context *c)
{
	contexts[c->get_name ()] = c;

	return c;
}

// Get the context registered under the given name, or nullptr when absent
::Wenv::Context * Display::get_context (const std::string & n)
{
	if (contexts.find (n) == contexts.end ())
	{
		return nullptr;
	}

	return contexts[n];
}

// Register a named modal (replacing any previous one under the same name)
::Wenv::Display::Modal *Display::add_modal (const std::string &n, ::Wenv::Display::Modal *m)
{
	modals[n] = m;

	return m;
}

// Get the modal registered under the given name, or nullptr when absent
::Wenv::Display::Modal *Display::get_modal (const std::string &n)
{
	if (modals.find (n) == modals.end ())
	{
		return nullptr;
	}

	return modals[n];
}

// Put the given title, text and buttons into the "modal" context, mark the
// named modal as the current one and redraw the display
void Display::show_modal
(
	const std::string &modal_name,
	const std::wstring &title,
	const std::wstring &text,
	const std::vector<::Wenv::Display::ModalButton> &buttons,
	const std::string &border_color,
	const std::string &title_color,
	const std::string &text_color
)
{
	auto modal = get_modal (modal_name);
	if (modal == nullptr)
	{
		return;
	}

	// The "modal" context carries the content of the currently shown modal;
	// the apps attached to its inner blocks read from it
	auto ctx = get_context ("modal");
	if (ctx == nullptr)
	{
		return;
	}

	ctx->set ("modal-title", new std::wstring { title });
	ctx->set ("modal-text", new std::wstring { text });
	ctx->set ("modal-buttons", new std::vector<::Wenv::Display::ModalButton> { buttons });
	// The first button is the active one when the modal appears
	ctx->set ("modal-active-button", new int { 0 });
	ctx->set ("modal-border-color", new std::string { border_color });
	ctx->set ("modal-title-color", new std::string { title_color });
	ctx->set ("modal-text-color", new std::string { text_color });

	current_modal = modal;

	// Redraw the display so the modal appears on top of the current contents
	if (window != nullptr && window->container_width > 0 && window->container_height > 0)
	{
		resize ((size_t) window->container_width, (size_t) window->container_height);
	}
}

// Get the global context shared across all displays (owned by the window)
::Wenv::Context *Display::get_persistent_context ()
{
	if (window == nullptr)
	{
		return nullptr;
	}

	return window->persistent_context;
}

} // namespace Wenv::Display
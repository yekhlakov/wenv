#include "Window.h"
#include "Display.h"
#include "../layout/Grid.h"
#include "../maxy/control/container.h"
#include "../apps/FuncMenu.h"

namespace Wenv::Display
{
void Window::initialize ()
{
	// File manager display
	auto d = new Display {};

    d->grid = new ::Wenv::Layout::Grid {};
    auto subgrid = new ::Wenv::Layout::Grid {};

    subgrid->add_row (0, 0, 100.);
    subgrid->add_column (0, 0, 33.);
    subgrid->add_column (0, 0, 33.);
    subgrid->add_column (0, 0, 34.);
    subgrid->add_block ({ 0, 0, 1, 1 }, 1);
    subgrid->add_block ({ 1, 0, 1, 1 }, 1);
    subgrid->add_block ({ 2, 0, 1, 1 }, 1);
    subgrid->is_exclusive = false;

    d->grid->add_row (0, 0, 50.0);
    d->grid->add_row (0, 0, 25.0);
    d->grid->add_row (3, 666, 0.0);
    d->grid->add_row (1, 1, 0.0);
    d->grid->add_column (0, 0, 33.0);
    d->grid->add_column (0, 0, 17.0);
    d->grid->add_column (0, 0, 50.0);
    d->grid->is_exclusive = true;

    auto c = maxy::control::Container {};
    auto func_menu = new ::Wenv::Apps::FuncMenu { c, L"MAIN MENU" };

    d->grid->add_block ({ 0, 0, 2, 1 }, 1);
    d->grid->add_block ({ 2, 0, 1, 1 }, 0);
    d->grid->add_block ({ 0, 1, 1, 2 }, 2, subgrid);
    d->grid->add_block ({ 1, 1, 2, 1 }, 1);
    d->grid->add_block ({ 1, 2, 2, 1 }, 2);
    d->grid->add_block ({ 0, 3, 3, 1 }, -1, nullptr, func_menu);

	displays.push_back (d);




	current_display = displays[0];
}
}

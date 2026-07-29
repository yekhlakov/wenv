#include "Window.h"
#include "Display.h"
#include "Palette.h"
#include "../layout/Grid.h"
#include "../maxy/control/container.h"
#include "../apps/Context.h"
#include "../apps/FileList.h"
#include "../apps/FuncMenu.h"


namespace Wenv::Display
{

std::wstring * get_current_directory ()
{
    auto s = new std::wstring { };
    s->resize (2000);
    auto n = GetCurrentDirectory (1999, s->data ());
    s->resize (n);
    return s;
}

void Window::initialize ()
{
    // Palette

    current_palette->colors.push_back ({0, 0x00808080}); // Highlight
    current_palette->colors.push_back ({ 0x00ffffff, 0 }); // Active

    current_palette->named_colors.insert ({ Palette::Default_color, 0 });
    current_palette->named_colors.insert ({ Palette::Highlight_color, 1 });
    current_palette->named_colors.insert ({ Palette::Active_element_color, 2 });

	// File manager display
	auto d = new Display {L"File manager"};

    auto grid = new ::Wenv::Layout::Grid {};
    grids.push_back (grid);

    d->grid = grid;

    {
        auto subgrid = new ::Wenv::Layout::Grid {};
        grids.push_back (subgrid);

        subgrid->add_row (3, 666, 0);
        subgrid->add_row (3, 3, 0);
        subgrid->add_column (0, 0, 50.);
        subgrid->add_column (0, 0, 50.);

        subgrid->is_exclusive = false;

        subgrid->context = new ::Wenv::Apps::Context { "file-manager-left-panel" };
        subgrid->context->set ("pwd", get_current_directory ());

        auto c1 = d->add_app (new ::Wenv::Apps::FileList { L"1" });
        auto c2 = d->add_app (new ::Wenv::Apps::FileList { L"2" });

        subgrid->context->set ("columns", new std::vector<::Wenv::Apps::App *> { c1, c2 });

        d->focused_app = c1;
        d->focused_app->focus ();

        subgrid->add_block ({ 0, 0, 1, 1 }, 1, nullptr, c1);
        subgrid->add_block ({ 1, 0, 1, 1 }, 1, nullptr, c2);
        subgrid->add_block ({ 0, 1, 2, 1 }, 1);

        grid->add_block ({ 0, 0, 1, 1 }, 2, subgrid);
    }

    {
        auto subgrid = new ::Wenv::Layout::Grid {};
        grids.push_back (subgrid);

        subgrid->add_row (3, 666, 0);
        subgrid->add_row (3, 3, 0);
        subgrid->add_column (0, 0, 50.);
        subgrid->add_column (0, 0, 50.);

        subgrid->is_exclusive = false;

        subgrid->context = new ::Wenv::Apps::Context { "file-manager-right-panel" };
        subgrid->context->set ("pwd", get_current_directory ());

        auto c1 = d->add_app (new ::Wenv::Apps::FileList { L"1" });
        auto c2 = d->add_app (new ::Wenv::Apps::FileList { L"2" });

        subgrid->context->set ("columns", new std::vector<::Wenv::Apps::App *> { c1, c2 });

        subgrid->add_block ({ 0, 0, 1, 1 }, 1, nullptr, c1);
        subgrid->add_block ({ 1, 0, 1, 1 }, 1, nullptr, c2);
        subgrid->add_block ({ 0, 1, 2, 1 }, 1);

        grid->add_block ({ 1, 0, 1, 1 }, 2, subgrid);
    }

    grid->add_row (0, 666, 0.0);
    grid->add_row (1, 1, 0.0);
    grid->add_column (0, 0, 50.0);
    grid->add_column (0, 0, 50.0);
    grid->is_exclusive = true;

    grid->add_block ({ 0, 1, 2, 1 }, -1, nullptr, d->add_app (new ::Wenv::Apps::FuncMenu { L"MAIN MENU" }));

	displays.push_back (d);


    set_display (0);
}
}

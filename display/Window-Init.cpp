#include "Window.h"
#include "Display.h"
#include "Palette.h"
#include "../layout/Grid.h"
#include "../maxy/control/container.h"
#include "../apps/Context.h"
#include "../apps/FuncMenu.h"

#include "../apps/FileManager/FileList.h"
#include "../apps/FileManager/FileListHeader.h"
#include "../apps/FileManager/FileInfoShort.h"

#include "../apps/FileEditor/FileEditor.h"
#include "../apps/FileEditor/FileEditorStatusBar.h"

namespace Wenv::Display
{

::Wenv::Layout::Grid *  Window::get_grid (const std::string &n)
{
    if (grids.find (n) == grids.end ())
    {
        return nullptr;
    }

    return grids[n];
}

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
    current_palette->colors.push_back ({ 0x000070aa, 0 }); // default
    current_palette->colors.push_back ({ 0, 0x000070aa }); // Highlight
    current_palette->colors.push_back ({ 0x0000aaff, 0 }); // Active

    current_palette->named_colors.insert ({ Palette::Default_color, 0 });
    current_palette->named_colors.insert ({ Palette::Highlight_color, 1 });
    current_palette->named_colors.insert ({ Palette::Active_element_color, 2 });


    auto func_menu = new ::Wenv::Apps::FuncMenu { L"MAIN MENU" };

    //--------------------------------------------------------------------------------------
    // File manager display
    //--------------------------------------------------------------------------------------

    {
        auto d = new Display { L"File manager" };

        auto grid = new ::Wenv::Layout::Grid {};
        grids.insert ({ "file-manager", grid });

        d->grid = grid;

        auto left_context = d->add_context (new ::Wenv::Apps::Context { "file-manager-left-panel" });
        left_context->set ("pwd", get_current_directory ());
        auto right_context = d->add_context (new ::Wenv::Apps::Context { "file-manager-right-panel" });
        right_context->set ("pwd", get_current_directory ());
        d->focused_context = left_context;

        auto medium_panel = new ::Wenv::Layout::Grid {};
        {
            grids.insert ({ "file-manager-medium-panel", medium_panel });

            medium_panel->add_row (1, 1, 0);
            medium_panel->add_row (3, 666, 0);
            medium_panel->add_row (3, 3, 0);
            medium_panel->add_column (0, 0, 50.);
            medium_panel->add_column (0, 0, 50.);

            medium_panel->is_exclusive = false;

            auto c0 = d->add_app (new ::Wenv::Apps::FileListHeader { L"h" });
            auto c1 = d->add_app (new ::Wenv::Apps::FileList { L"1" });
            auto c2 = d->add_app (new ::Wenv::Apps::FileList { L"2" });
            auto c3 = d->add_app (new ::Wenv::Apps::FileInfoShort { L"i" });
            d->add_app (func_menu);

            left_context->set ("app-group", new std::vector<::Wenv::Apps::App *> { c0, c1, c2, c3 });
            right_context->set ("app-group", new std::vector<::Wenv::Apps::App *> { c0, c1, c2, c3 });
            left_context->set ("focused-app", c1);
            right_context->set ("focused-app", c1);
            left_context->set ("focused-path", new std::string { "root.0.1" });
            right_context->set ("focused-path", new std::string { "root.1.1" });

            medium_panel->add_block ({ 0, 0, 2, 1 }, -1, nullptr, c0);
            medium_panel->add_block ({ 0, 1, 1, 1 }, 1, nullptr, c1);
            medium_panel->add_block ({ 1, 1, 1, 1 }, 1, nullptr, c2);
            medium_panel->add_block ({ 0, 2, 2, 1 }, 1, nullptr, c3);
        }

        grid->add_row (0, 666, 0.0);
        grid->add_row (1, 1, 0.0);
        grid->add_column (0, 0, 50.0);
        grid->add_column (0, 0, 50.0);
        grid->is_exclusive = true;

        grid->add_block ({ 0, 0, 1, 1 }, 2, medium_panel, nullptr, left_context);
        grid->add_block ({ 1, 0, 1, 1 }, 2, medium_panel, nullptr, right_context);
        grid->add_block ({ 0, 1, 2, 1 }, -1, nullptr, func_menu);

        add_display("file-manager", d);
    }

    //--------------------------------------------------------------------------------------
    // File viewer/editor display
    //--------------------------------------------------------------------------------------
    {
        auto d = new Display { L"File Editor Viewer" };

        auto grid = new ::Wenv::Layout::Grid {};
        grids.insert ({ "file-editor-viewer", grid });

        grid->add_row (1, 1, 0.0);
        grid->add_row (1, 666, 0.0);
        grid->add_row (1, 1, 0.0);
        grid->add_column (0, 0, 100.);
        grid->is_exclusive = true;

        auto status = d->add_app (new ::Wenv::Apps::FileEditorStatusBar { L"File Editor status" });
        auto editor = d->add_app (new ::Wenv::Apps::FileEditor { L"File Editor" });
        d->add_app (func_menu);

        grid->add_block ({ 0,0,1,1 }, -1, nullptr, status);
        grid->add_block ({ 0,1,1,1 }, -1, nullptr, editor);
        grid->add_block ({ 0,2,1,1 }, -1, nullptr, func_menu);
        grid->context = d->add_context (new ::Wenv::Apps::Context { "file-editor" });
        d->focused_context = grid->context;
        grid->context->set ("focused-app", editor);
        grid->context->set ("app-group", new std::vector<::Wenv::Apps::App *> { status, editor, func_menu });
        grid->context->set ("focused-path", "root.1");

        d->grid = grid;

        add_display ("file-editor", d);
    }

    set_display ("file-manager");
}
}

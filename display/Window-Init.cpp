#include "Window.h"
#include "Display.h"
#include "Modal.h"
#include "Palette.h"
#include "../layout/Grid.h"
#include "../maxy/control/container.h"
#include "../Context.h"
#include "../apps/FuncMenu/FuncMenu.h"

#include "../apps/FileManager/FileList.h"
#include "../apps/FileManager/FileListHeader.h"
#include "../apps/FileManager/FileInfoShort.h"

#include "../apps/FileEditor/File.h"
#include "../apps/FileEditor/FileEditor.h"
#include "../apps/FileEditor/FileEditorStatusBar.h"

#include "../apps/Modal/ModalTitle.h"
#include "../apps/Modal/ModalText.h"
#include "../apps/Modal/ModalButtons.h"

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

// Build the default func menu command list: the single F10 "Exit" command
// closes the current display (or the whole application when it is the only
// display left)
std::vector<::Wenv::Apps::FuncMenuCommand> * make_func_menu_default_commands ()
{
	auto commands = new std::vector<::Wenv::Apps::FuncMenuCommand> (12);

	(*commands)[9] = {
		L"Exit",

		[] (::Wenv::Display::Display *display, ::Wenv::Context *)
		{
			if (!display->window->pop_display ())
			{
				DestroyWindow (display->window->hwnd);
			}
			else
			{
				display->window->invalidate_modified ();
			}
		}
	};

	return commands;
}

// Build the default func menu command list of the file editor: in addition to
// the common F10 "Exit" it has the F3 "TabSz" command, which cycles the tab
// width of the editor. The func menu redraws the apps of the context after a
// command has run, so the new width shows up in the editor and its status bar
std::vector<::Wenv::Apps::FuncMenuCommand> * make_func_menu_file_editor_commands ()
{
	auto commands = make_func_menu_default_commands ();

	(*commands)[2] = {
		L"TabSz",

		[] (::Wenv::Display::Display *display, ::Wenv::Context *)
		{
			::Wenv::Apps::cycle_tab_width (display->get_persistent_context ());
		}
	};

	return commands;
}

// Build the default func menu command list of the file list: in addition to
// the common F10 "Exit" it has the F3 "View" and F4 "Edit" commands, which show
// the file selected in the panel in the editor - either closed for editing or
// open for it, which its status bar shows as a lock
std::vector<::Wenv::Apps::FuncMenuCommand> * make_func_menu_file_list_commands ()
{
	auto commands = make_func_menu_default_commands ();

	(*commands)[2] = {
		L"View",

		[] (::Wenv::Display::Display *display, ::Wenv::Context *context)
		{
			::Wenv::Apps::show_selected_file (display, context, false);
		}
	};

	(*commands)[3] = {
		L"Edit",

		[] (::Wenv::Display::Display *display, ::Wenv::Context *context)
		{
			::Wenv::Apps::show_selected_file (display, context, true);
		}
	};

	return commands;
}

void Window::initialize ()
{
    // Palette
	/*
    current_palette->colors.push_back ({ 0x000070aa, 0, 0, 0x000070aa }); // default
    current_palette->colors.push_back ({ 0x0030c0ff, 0, 0x0030c0ff, 0x000070aa }); // Active
    current_palette->colors.push_back ({ 0x00004460, 0, 0x00004060, 0x000070aa }); // Dark
    current_palette->colors.push_back ({ 0x0000aaaa, 0, 0x00006066, 0x000070aa }); // Quote aka String
    current_palette->colors.push_back ({ 0x008877aa, 0, 0x00554466, 0x000070aa }); // Number
    current_palette->colors.push_back ({ 0x00667788, 0, 0x00334455, 0x000070aa }); // Alter aka Define
    current_palette->colors.push_back ({ 0x000040aa, 0, 0x00003099, 0x000070aa }); // Warning
	*/

    int bg = 0x000070aa;

    current_palette->colors.push_back ({ bg, 0, 0, bg }); // default
    current_palette->colors.push_back ({ 0x0000ccff, 0, 0x0000ccff, bg }); // Active
    current_palette->colors.push_back ({ 0x00004466, 0, 0x00004466, bg }); // Dark
    current_palette->colors.push_back ({ 0x0000bbaa, 0, 0x00005540, bg }); // Quote aka String
    current_palette->colors.push_back ({ 0x00995566, 0, 0x00663344, bg }); // Number
    current_palette->colors.push_back ({ 0x00778899, 0, 0x00445055, bg }); // Alter aka Define
    current_palette->colors.push_back ({ 0x000070ff, 0, 0x00000077, bg }); // Warning

    current_palette->named_colors.insert ({ Palette::Default_color, 0 });
    current_palette->named_colors.insert ({ Palette::Active_element_color, 1 });
    current_palette->named_colors.insert ({ Palette::Dark_element_color, 2 });
    current_palette->named_colors.insert ({ Palette::Quote_element_color, 3 });
    current_palette->named_colors.insert ({ Palette::Number_element_color, 4 });
    current_palette->named_colors.insert ({ Palette::Alter_element_color, 5 });
    current_palette->named_colors.insert ({ Palette::Warning_element_color, 6 });


    auto func_menu = new ::Wenv::Apps::FuncMenu { L"MAIN MENU" };

    //--------------------------------------------------------------------------------------
    // File manager display
    //--------------------------------------------------------------------------------------

    {
        auto d = new Display { L"File manager" };

        auto grid = new ::Wenv::Layout::Grid {};
        grids.insert ({ "file-manager", grid });

        d->grid = grid;

        // The parent context shared by both panels: it holds the default
        // func menu command list (F3/F4 show the selected file in the editor,
        // F10 exits the application)
        auto main_context = d->add_context (new ::Wenv::Context { "file-manager" });
        main_context->set ("func_menu.default", make_func_menu_file_list_commands ());

        auto left_context = d->add_context (new ::Wenv::Context { "file-manager-left-panel" });
        left_context->parent = main_context;
        left_context->set ("pwd", get_current_directory ());
        auto right_context = d->add_context (new ::Wenv::Context { "file-manager-right-panel" });
        right_context->parent = main_context;
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

            left_context->set ("app-group", new std::vector<::Wenv::Apps::App *> { c0, c1, c2, c3, func_menu });
            right_context->set ("app-group", new std::vector<::Wenv::Apps::App *> { c0, c1, c2, c3, func_menu });
            left_context->set ("focused-app", c1);
            right_context->set ("focused-app", c1);
            left_context->set ("focused-path", new std::string { "root.0.1" });
            right_context->set ("focused-path", new std::string { "root.1.1" });
	    left_context->set ("func-menu", func_menu);
	    right_context->set ("func-menu", func_menu);


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

        // The modal box slots: the apps attached to its grid blocks draw the
        // modal title, the modal text and the modal buttons from the shared
        // "modal" context, so the modal itself carries no content
        d->add_context (new ::Wenv::Context { "modal" });
        auto modal_title = d->add_app (new ::Wenv::Apps::ModalTitle { L"modal-title" });
        auto modal_text = d->add_app (new ::Wenv::Apps::ModalText { L"modal-text" });
        auto modal_buttons = d->add_app (new ::Wenv::Apps::ModalButtons { L"modal-buttons" });

        auto test_modal = new ::Wenv::Display::Modal {
            modal_title,
            modal_text,
            modal_buttons
        };
        d->add_modal ("test", test_modal);

        // A test modal that is always displayed on top of the display contents
        std::vector<::Wenv::Display::ModalButton> buttons {
            { L"OK", nullptr }
        };
        d->show_modal (
            "test",
            L"Test modal",
            L"This is a test modal. It is drawn on top of the display contents, centered in the window.",
            buttons
        );
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
        grid->context = d->add_context (new ::Wenv::Context { "file-editor" });
        d->focused_context = grid->context;
        grid->context->set ("focused-app", editor);
        grid->context->set ("app-group", new std::vector<::Wenv::Apps::App *> { status, editor, func_menu });
        grid->context->set ("focused-path", new std::string { "root.1" });
        grid->context->set ("status-bar", status);
	grid->context->set ("func-menu", func_menu);

	// The default command list of the editor: F3 cycles the tab width,
	// F10 exits the display
	grid->context->set ("func_menu.default", make_func_menu_file_editor_commands ());

        d->grid = grid;

        add_display ("file-editor", d);
    }

    set_display ("file-manager");
}
}

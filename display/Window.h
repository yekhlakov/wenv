#pragma once

#include <windows.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace Wenv {
class Context;
}

namespace Wenv::Apps {
class App;
}

namespace Wenv::Layout {
struct Grid;
}

namespace Wenv::Display {

struct Display;
struct Palette;

struct Window {
    HWND hwnd;
    HDC hdc;
    std::wstring font_name;

    // sizes of character
    int char_width;
    int char_height;

    // Window size
    int container_width;
    int container_height;

    std::unordered_map<std::string, Display *> displays;
    std::unordered_map<std::string, ::Wenv::Layout::Grid *> grids;

    std::vector<Display *> display_stack;

    Display* current_display;
    Palette* current_palette;

    // Input state
    bool key_state[256];
    int mouse_x = 0;
    int mouse_y = 0;

    HFONT hFont;
    std::vector<std::wstring> monospace_fonts;

    // Global context shared across all displays
    ::Wenv::Context *persistent_context = nullptr;

    Window(HINSTANCE hInstance, std::wstring title, std::wstring className);
    ~Window();

    // Create displays, grids, apps etc
    void initialize ();

    void set_font (std::wstring name);
    void add_display (const std::string &n, Display *d);
    void set_display (const std::string & n);
    bool pop_display ();
    Display *get_display (const std::string &n);

    // Window message handlers
    void handle_resizing (WPARAM wParam, LPARAM lParam);
    void handle_resize (WPARAM wParam, LPARAM lParam);
    void handle_keydown (WPARAM wParam, LPARAM lParam);
    void handle_keyup (WPARAM wParam, LPARAM lParam);
    void handle_mousemove (WPARAM wParam, LPARAM lParam);
    void handle_mouse_click (WPARAM wParam, LPARAM lParam);

    // The current modifier state: 1 = Ctrl, 2 = Shift, 4 = Alt
    int current_mods () const;

    // Dispatch a key press or key release to the apps: while a modal is visible
    // every event goes to the modal apps only, otherwise to the focused app
    void dispatch_key_event (WPARAM wParam, int mods, bool pressed);

    // Call the keydown or keyup handler of each app, bound to the given context,
    // until an app consumes the event. Returns whether the event was consumed
    bool forward_key_to_apps (
        const std::vector<::Wenv::Apps::App *> &apps,
        ::Wenv::Context *ctx,
        WPARAM wParam,
        int mods,
        bool pressed);

    // The current pressed state of the given key
    bool get_key_state (int key) const;

    void activate_current ();

    // Draw only the characters whose cells intersect the update rectangle;
    // called from the WM_PAINT handler, the only place drawing to the window
    void draw (HDC hdc, const RECT &update_rect);

    // Invalidate the minimal window rectangle that covers all the characters
    // of the current display marked as modified, then clear the flags; the
    // characters are actually drawn by the WM_PAINT handler
    void invalidate_modified ();

    ::Wenv::Layout::Grid *get_grid (const std::string &n);

    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
};

} // namespace Wenv::Display

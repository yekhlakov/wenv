#pragma once

#include <windows.h>
#include <string>
#include <unordered_map>
#include <vector>

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

    Display* current_display;
    Palette* current_palette;

    // Input state
    bool key_state[256];
    int mouse_x = 0;
    int mouse_y = 0;

    HFONT hFont;
    std::vector<std::wstring> monospace_fonts;

    Window(HINSTANCE hInstance, std::wstring title, std::wstring className);
    ~Window();

    // Create displays, grids, apps etc
    void initialize ();

    void set_font (std::wstring name);
    void add_display (const std::string &n, Display *d);
    void set_display (const std::string & n);
    Display *get_display (const std::string &n);

    // Window message handlers
    void handle_resizing (WPARAM wParam, LPARAM lParam);
    void handle_resize (WPARAM wParam, LPARAM lParam);
    void handle_keydown (WPARAM wParam, LPARAM lParam);
    void handle_keyup (WPARAM wParam, LPARAM lParam);
    void handle_mousemove (WPARAM wParam, LPARAM lParam);

    void draw(HDC hdc);

    ::Wenv::Layout::Grid *get_grid (const std::string &n);

    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
};

} // namespace Wenv::Display

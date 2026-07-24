#pragma once

#include <windows.h>
#include <string>
#include <vector>

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

    std::vector<Display *> displays;
    Display* current_display;
    Palette* current_palette;

    HFONT hFont;
    std::vector<std::wstring> monospace_fonts;

    Window(HINSTANCE hInstance, std::wstring title, std::wstring className);
    ~Window();

    // Create displays, grids, apps etc
    void initialize ();

    void set_font(std::wstring name);
    void handle_resizing (WPARAM wParam, LPARAM lParam);
    void handle_resize (WPARAM wParam, LPARAM lParam);
    void draw(HDC hdc);

    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
};

} // namespace Wenv::Display

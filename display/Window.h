#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "Display.h"

namespace Wenv::Display {

struct Window {
    HWND hwnd;
    HDC hdc;
    std::wstring font_name;
    int char_width;
    int char_height;
    Display* current_display;
    HFONT hFont;
    std::vector<std::wstring> monospace_fonts;

    Window(HINSTANCE hInstance, std::wstring title, std::wstring className);
    ~Window();

    void set_font(std::wstring name);
    void resize();
    void draw(HDC hdc);

    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
};

} // namespace Wenv::Display

#pragma once

namespace Wenv::Display {

struct Character {
    wchar_t value;
    int palette_color;
    int fg_color;
    int bg_color;
    bool modified;

    void set(wchar_t val, int pcolor = 0, int fg = 0, int bg = 0);
};

} // namespace Wenv::Display

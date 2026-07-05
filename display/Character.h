#pragma once

namespace Wenv::Display {

struct Character {
    wchar_t value;
    int fg_color;
    int bg_color;
    bool modified;

    void set(wchar_t val, int fg, int bg);
};

} // namespace Wenv::Display

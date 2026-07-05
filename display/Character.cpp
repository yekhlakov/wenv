#include "Character.h"

namespace Wenv::Display {

void Character::set(wchar_t val, int fg, int bg) {
    value = val;
    fg_color = fg;
    bg_color = bg;
    modified = true;
}

} // namespace Wenv::Display

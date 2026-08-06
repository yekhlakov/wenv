#include <format>
#include <random>
#include "Display.h"
#include "../Layout/Grid.h"
#include "../apps/FuncMenu.h"
#include "../apps/Context.h"

namespace Wenv::Display
{

Display::Display (const std::wstring &n) :
    name { n }
{
   
}

Display::~Display ()
{
    if (grid != nullptr)
    {
        delete grid;
    }
}

void Display::resize (size_t width, size_t height)
{
    data.resize (height);
    std::mt19937 gen (std::random_device {}());
    std::uniform_int_distribution<int> ascii_dist (0x0020, 0x007E);
    std::uniform_int_distribution<int> cyrillic_dist (0x0400, 0x045F);
    std::uniform_int_distribution<int> region_dist (0, 1);
    std::uniform_int_distribution<int> palette_dist (0, 16);
    //std::uniform_int_distribution<int> color_dist(0, 0xFFFFFF);


    for (size_t y = 0; y < height; ++y) {
        data[y].resize (width);
        for (size_t x = 0; x < width; ++x) {
            wchar_t ch;
            //int region = region_dist(gen);
            //if (region == 0) 
            //ch = static_cast<wchar_t>(ascii_dist(gen));
            //else 
            ch = static_cast<wchar_t> (cyrillic_dist (gen));

            data[y][x].set (ch, palette_dist (gen));
        }
    }

    // Char buffer to hold boundary texts
    std::vector<std::vector<wchar_t>> buffer (height, std::vector<wchar_t> (width + 1, L' '));

    grid->bake ({ 0, 0, (int) width, (int) height }, buffer, "root");

    draw_grid (*grid, "root");
}

void Display::draw_grid (::Wenv::Layout::Grid &grid, std::string path, ::Wenv::Apps::Context * ctx)
{
    // First draw all boundaries
    int bnum = 0;
    for (auto &b : grid.blocks)
    {
        auto bpath = std::format ("{}.{}", path, bnum++);
        draw_block_boundary (b, bpath);
        if (b.grid != nullptr)
        {
            // If the block has nested grid, recurse
            draw_grid (*b.grid, bpath, b.get_context (ctx));
        }
    }
    
    // Then draw app conents because the conent may overwrite some boundaries
    bnum = 0;
    for (auto &b : grid.blocks)
    {
        auto bpath = std::format ("{}.{}", path, bnum++);
        if (b.app != nullptr)
        {
            // Otherwise if the block has an attached app, ask the app to draw its contents
            b.app->with_context (b.get_context (ctx))
                ->draw (*this, bpath, b.get_client_dimensions (bpath));
        }
    }
}

// Set the palette color (or real color) for subsequent output operations
void Display::set_color (int pc, int fg, int bg)
{
	current_palette_color = pc;
	current_foreground_color = fg;
	current_background_color = bg;
}


::Wenv::Apps::App *Display::add_app (::Wenv::Apps::App *a)
{
    all_apps.push_back (a);
    if (a->wants_all_keypresses ())
    {
        listening_apps.push_back (a);
    }

    return a;
}


::Wenv::Apps::Context *Display::add_context (::Wenv::Apps::Context *c)
{
    contexts[c->get_name ()] = c;

    return c;
}

::Wenv::Apps::Context * Display::get_context (const std::string & n)
{
    if (contexts.find (n) == contexts.end ())
    {
        return nullptr;
    }

    return contexts[n];
}

} // namespace Wenv::Display

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "Character.h"
#include "Palette.h"
#include "..\Types.h"

namespace Wenv::Display {

struct Window;
struct Modal;

struct Display
{
	inline static const int PF_LEFT = 0;
	inline static const int PF_CENTER = 1;
	inline static const int PF_RIGHT = 2;
	inline static const int PF_TOP = 0;
	inline static const int PF_VCENTER = 4;
	inline static const int PF_BOTTOM = 8;
	inline static const int PF_CLIP = 0;
	inline static const int PF_NOCLIP = 16;
	inline static const int PF_ERASE_BACKGROUND = 32;

    std::vector<std::vector<Character>> data;

	std::wstring name;

	Palette *current_palette = nullptr;
    int current_palette_color = 0;
    int current_foreground_color = 0;
    int current_background_color = 0;

	::Wenv::Layout::Grid * grid = nullptr;
	std::vector<::Wenv::Apps::App *> all_apps;
	std::vector<::Wenv::Apps::App *> listening_apps;
	std::unordered_map<std::string, ::Wenv::Context *> contexts;

	// The modal currently drawn on top of the display contents (nullptr = no modal)
	::Wenv::Display::Modal *current_modal = nullptr;

	// Named modals available to the display
	std::unordered_map<std::string, ::Wenv::Display::Modal *> modals;

	::Wenv::Apps::App *focused_app = nullptr;
	::Wenv::Context *focused_context = nullptr;

	::Wenv::Apps::App *add_app (::Wenv::Apps::App *a);
	::Wenv::Context * add_context (::Wenv::Context *c);
	::Wenv::Context *get_context (const std::string &n);
	::Wenv::Display::Modal *add_modal (const std::string &n, ::Wenv::Display::Modal *m);
	::Wenv::Display::Modal *get_modal (const std::string &n);

	// Get the global context shared across all displays (owned by the window)
	::Wenv::Context *get_persistent_context ();

	Window *window = nullptr;

	// The current pressed state of the given key (proxied to the window)
	bool get_key_state (int key) const;

	Display (const std::wstring &n);
	~Display ();

    // Resize the display
    void resize (size_t width, size_t height);


	Display *with_palette (Palette *p);
	Display *with_color (const std::string &n, bool is_highlight = false);

	// OUTPUT

	// Set the palette color (or real color) for subsequent output operations
 	void set_color (int pc = 0, int fg = 0, int bg = 0);

	// Print character to specified position
	void print_char (size_t pos, size_t ln, wchar_t ch);

	// Print line left to right starting from specified position
	void print_line (size_t pos, size_t ln, const std::wstring & s);

	// Extended printing
	void print_line (Rect container, const std::wstring &s, int flags = 0);

	// Compute the dimensions of the minimal rectangle (in characters) this text can be
	// printed into, with word wrapping. The result may exceed the max limits if the text
	// forces it; the limits only add a penalty to the area when options are compared.
	// If out_lines is not null, it is populated with the lines of the wrapped text.
	Rect get_min_rectangle (const std::wstring &text, int min_width = 1, int min_height = 1, int max_width = 666, int max_height = 666, std::vector<std::wstring> *out_lines = nullptr);

	// Print line top to bottom starting from specified position
	void print_line_v (size_t pos, size_t ln, const std::wstring & s);

	// Draw a grid (recursively). All its boundaries are drawn in the given palette
	// color (highlight variant if requested), ignoring the ambient drawing colors
	void draw_grid (::Wenv::Layout::Grid &grid, std::string path, ::Wenv::Context *ctx = nullptr, const std::string &color = ::Wenv::Display::Palette::Default_color, bool highlight = false);

	// Draw a grid block (using its boundary strings) in the given palette color
	void draw_block_boundary (::Wenv::Layout::Block &b, const std::string &path, const std::string &color = ::Wenv::Display::Palette::Default_color, bool highlight = false);

	// Redraw grid boundaries that cross the given display row within the given
	// horizontal span (recursively). App contents are not redrawn; use this to
	// restore borders that a title was drawn over. The boundaries are drawn in the
	// given palette color, ignoring the ambient drawing colors
	void redraw_boundaries_on_row (int y, int x_begin, int x_end, const std::string &color = ::Wenv::Display::Palette::Default_color);
	void redraw_boundaries_on_row (::Wenv::Layout::Grid &grid, std::string path, int y, int x_begin, int x_end, const std::string &color = ::Wenv::Display::Palette::Default_color);

	// Draw rectangular box with constant border
	void draw_box (size_t pos, size_t ln, size_t w, size_t h, int btype);
};

} // namespace Wenv::Display

#pragma once

#include <Windows.h>
#include <cstdint>
#include <list>
#include <string>
#include <vector>
#include "../../Types.h"

namespace Wenv::Apps
{

struct FileLine
{
	std::string raw_data;

	// The newline that follows the line content in the file ("\\r\\n" or
	// "\\n"); empty for the last line when the file does not end with a
	// newline
	std::string newline;
};

// The total number of lines is not known until the entire file is loaded.
// Use this as the line count when it is still unknown.
constexpr int UNKNOWN_LINE_COUNT = -1;

// Expand tabs to tab_width-column stops and convert the raw UTF-8 line to display text.
// tab_width must be positive.
// Returns the expanded string and a list of (position, length) pairs for each tab expansion.
// position = column of the first space of the tab expansion, length = expansion width including the arrow.
std::pair<std::wstring, std::vector<std::pair<int, int>>> expand_tabs (const std::string &line, int tab_width);

// The byte length of the utf-8 character sequence starting with the given
// lead byte; a malformed lead byte is treated as a single byte
int utf8_char_length (char lead);

// The tab width of the file editor, stored in the given persistent context, so
// it survives restarts. A fresh environment uses 4. The width is kept within
// the 1..9 range the display assumes
int * get_tab_width (::Wenv::Context *persistent_context);

// Set the next tab width of the given persistent context: 4 -> 8 -> 2 -> 4 ...
void cycle_tab_width (::Wenv::Context *persistent_context);

class File;

// The raw length of the given line of the file, before the tab expansion;
// a line beyond the loaded content has length zero
int file_line_length (File *file, int line);

// The display position of the given raw position in the given line: the width
// of the tab-expanded part of the line before the position. A tab under the
// position is displayed at the beginning of its expansion, the positions
// beyond the line end occupy one cell each
int file_line_display_pos (File *file, int line, int pos, int tab_width);

// The inverse of file_line_display_pos: the raw position in the given line of
// the given display position. The largest raw position displayed not past the
// given display position is taken, so a display position within a tab
// expansion is converted to the beginning of the tab; the display positions
// past the line end occupy one cell each
int file_line_raw_pos (File *file, int line, int display_pos, int tab_width);

class File
{
	std::wstring path;
	HANDLE handle = INVALID_HANDLE_VALUE;
	std::uint64_t total_size = 0;
	std::uint64_t bytes_read = 0;
	int bom_offset = 0;
	bool fully_loaded = false;

	// The tab width longest_expanded was measured with
	int measured_tab_width = 4;

public:
	std::list<FileLine> lines;
	std::size_t longest_expanded = 0;

	// The newline of the file as determined on opening: the most common of
	// the first few newlines of the content, the Windows CRLF by default
	std::string newline = "\r\n";

	File (const std::wstring &path);
	~File ();

	File (const File &) = delete;
	File &operator= (const File &) = delete;

	bool is_loaded () const { return handle != INVALID_HANDLE_VALUE || fully_loaded; }
	bool is_fully_loaded () const { return fully_loaded; }
	std::uint64_t get_file_size () const { return total_size; }
	int get_line_count () const { return fully_loaded ? (int) lines.size () : UNKNOWN_LINE_COUNT; }

	// Expand the line with the current tab width and widen longest_expanded
	// accordingly; must be called after the line content has changed
	void measure (const FileLine &line);

	// Widen longest_expanded to the given tab width, re-measuring the loaded
	// content when the width differs from the one it was measured with
	void set_tab_width (int tab_width);

	// If more data should be loaded for the given visible top line,
	// load the next chunk. Safe to call every redraw.
	void ensure_loaded (int top_line, int visible_height);

	// Save the current content of the file: the remaining content is
	// loaded first, then the lines are written with their own newlines,
	// the BOM of the source file (if any) is preserved. Returns true on
	// success
	bool save ();

private:
	// Determine the newline of the file from the beginning of its content
	void detect_newline (const std::string &raw);

	// Load the remaining content of the file; saving needs the whole of it
	void load_all ();

	void load_more ();
};

}
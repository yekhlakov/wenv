#pragma once

#include <Windows.h>
#include <cstdint>
#include <list>
#include <string>

namespace Wenv::Apps
{

struct FileLine
{
	std::string raw_data;
};

// The total number of lines is not known until the entire file is loaded.
// Use this as the line count when it is still unknown.
constexpr int UNKNOWN_LINE_COUNT = -1;

// Expand tabs to 4-column stops and convert the raw UTF-8 line to display text
std::wstring expand_tabs (const std::string &line);

class File
{
	std::wstring path;
	HANDLE handle = INVALID_HANDLE_VALUE;
	std::uint64_t total_size = 0;
	std::uint64_t bytes_read = 0;
	int bom_offset = 0;
	bool fully_loaded = false;

public:
	std::list<FileLine> lines;
	std::size_t longest_expanded = 0;

	File (const std::wstring &path);
	~File ();

	File (const File &) = delete;
	File &operator= (const File &) = delete;

	bool is_loaded () const { return handle != INVALID_HANDLE_VALUE || fully_loaded; }
	bool is_fully_loaded () const { return fully_loaded; }
	std::uint64_t get_file_size () const { return total_size; }
	int get_line_count () const { return fully_loaded ? (int) lines.size () : UNKNOWN_LINE_COUNT; }

	// If more data should be loaded for the given visible top line,
	// load the next chunk. Safe to call every redraw.
	void ensure_loaded (int top_line, int visible_height);

private:
	void load_more ();
};

}
#include <algorithm>
#include <array>
#include <iterator>
#include <string>
#include "../../Context.h"
#include "../../maxy/strings.h"
#include "File.h"

namespace Wenv::Apps
{

static constexpr std::uint64_t ONE_MB = 1024 * 1024;

std::pair<std::wstring, std::vector<std::pair<int, int>>> expand_tabs (const std::string &line, int tab_width)
{
	auto wide = maxy::strings::utf8towchar (line);
	std::wstring out;
	std::vector<std::pair<int, int>> tab_spans;
	int col = 0;

	for (auto ch : wide)
	{
		if (ch == L'\t')
		{
			int spaces = tab_width - (col % tab_width);
			int pos = (int) out.size ();
			out.append (spaces, L' ');
			tab_spans.push_back ({ pos, spaces });
			col += spaces;
		}
		else
		{
			out += ch;
			col++;
		}
	}

	return { out, tab_spans };
}

int * get_tab_width (::Wenv::Context *persistent_context)
{
	auto tab_width = persistent_context->get<int> ("tab-width", [] () { return new int { 4 }; });

	// A width outside the 1..9 range the display assumes would break the tab
	// expansion, so a stale or damaged stored value is clamped
	*tab_width = std::clamp (*tab_width, 1, 9);

	return tab_width;
}

void cycle_tab_width (::Wenv::Context *persistent_context)
{
	// The widths the file editor cycles through
	static const std::array<int, 3> widths { 4, 8, 2 };

	auto tab_width = get_tab_width (persistent_context);
	auto current = std::find (widths.begin (), widths.end (), *tab_width) - widths.begin ();

	*tab_width = widths[(current + 1) % widths.size ()];
}

int file_line_length (File *file, int line)
{
	if (file == nullptr || line < 0 || line >= (int) file->lines.size ())
	{
		return 0;
	}

	auto it = file->lines.begin ();
	std::advance (it, line);

	return (int) it->raw_data.size ();
}

int file_line_display_pos (File *file, int line, int pos, int tab_width)
{
	// A position on a line that is not in the file (beyond its end) has no
	// tab expansion to account for; a negative position is clamped to zero
	if (file == nullptr || line < 0 || line >= (int) file->lines.size () || pos <= 0)
	{
		return max (pos, 0);
	}

	auto it = file->lines.begin ();
	std::advance (it, line);

	// The display width of the tab-expanded part of the line before the
	// position, so a tab under the position is displayed at the beginning
	// of its expansion; the positions beyond the line end occupy one cell each
	auto raw_len = (int) it->raw_data.size ();
	auto within = min (pos, raw_len);

	return (int) expand_tabs (it->raw_data.substr (0, within), tab_width).first.size ()
		+ max (0, pos - raw_len);
}

File::File (const std::wstring &file_path)
	: path { file_path }
{
	handle = CreateFileW (
		path.c_str (),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (handle == INVALID_HANDLE_VALUE)
	{
		return;
	}

	LARGE_INTEGER size;
	if (!GetFileSizeEx (handle, &size))
	{
		CloseHandle (handle);
		handle = INVALID_HANDLE_VALUE;
		return;
	}

	total_size = (std::uint64_t) size.QuadPart;

	// Determine how much to read initially
	auto initial_read = total_size <= ONE_MB ? total_size : ONE_MB;
	if (initial_read == 0)
	{
		fully_loaded = true;
		CloseHandle (handle);
		handle = INVALID_HANDLE_VALUE;
		return;
	}

	// Read the first chunk
	std::string raw ((size_t) initial_read, '\0');
	DWORD actually_read { 0 };
	if (!ReadFile (handle, raw.data (), (DWORD) initial_read, &actually_read, nullptr) || actually_read == 0)
	{
		CloseHandle (handle);
		handle = INVALID_HANDLE_VALUE;
		return;
	}

	raw.resize (actually_read);

	// Detect and skip UTF-8 BOM
	if (raw.size () >= 3
		&& (unsigned char) raw[0] == 0xEF
		&& (unsigned char) raw[1] == 0xBB
		&& (unsigned char) raw[2] == 0xBF)
	{
		bom_offset = 3;
		raw.erase (0, 3);
	}

	// The number of raw bytes consumed from the file for the loaded content
	bytes_read = actually_read - bom_offset;

	// Split the raw bytes into FileLine's, keeping the raw byte content
	std::string cur;
	for (auto c : raw)
	{
		if (c == '\n')
		{
			lines.push_back ({ cur });
			cur.clear ();
		}
		else if (c != '\r')
		{
			cur += c;
		}
	}

	// Last partial line (no trailing newline)
	lines.push_back ({ cur });

	// Determine the longest expanded line of the loaded content
	for (auto &l : lines)
	{
		measure (l);
	}

	// If the file is small enough, we are done
	if (total_size <= ONE_MB)
	{
		fully_loaded = true;
		CloseHandle (handle);
		handle = INVALID_HANDLE_VALUE;
	}
}

File::~File ()
{
	if (handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle (handle);
	}
}

void File::set_tab_width (int tab_width)
{
	if (tab_width == measured_tab_width)
	{
		return;
	}

	measured_tab_width = tab_width;

	// The expanded width of the content depends on the tab width, so the
	// already loaded lines must be measured again from scratch
	longest_expanded = 0;

	for (auto &l : lines)
	{
		measure (l);
	}
}

void File::measure (const FileLine &line)
{
	longest_expanded = max (longest_expanded, expand_tabs (line.raw_data, measured_tab_width).first.size ());
}

void File::ensure_loaded (int top_line, int visible_height)
{
	if (fully_loaded)
	{
		return;
	}

	// Load more when the viewport reaches near the end of loaded content
	if (top_line + visible_height >= (int) lines.size ())
	{
		load_more ();
	}
}

void File::load_more ()
{
	if (fully_loaded || handle == INVALID_HANDLE_VALUE)
	{
		return;
	}

	// How many bytes remain in the file
	auto remaining = total_size - bytes_read - bom_offset;
	if (remaining == 0)
	{
		fully_loaded = true;
		CloseHandle (handle);
		handle = INVALID_HANDLE_VALUE;
		return;
	}

	auto chunk_size = remaining < ONE_MB ? remaining : ONE_MB;

	std::string raw ((size_t) chunk_size, '\0');
	DWORD actually_read { 0 };

	// Seek to the correct position (past BOM + already read content)
	LARGE_INTEGER offset;
	offset.QuadPart = (LONGLONG) (bytes_read + bom_offset);
	if (!SetFilePointerEx (handle, offset, nullptr, FILE_BEGIN))
	{
		fully_loaded = true;
		CloseHandle (handle);
		handle = INVALID_HANDLE_VALUE;
		return;
	}

	if (!ReadFile (handle, raw.data (), (DWORD) chunk_size, &actually_read, nullptr) || actually_read == 0)
	{
		fully_loaded = true;
		CloseHandle (handle);
		handle = INVALID_HANDLE_VALUE;
		return;
	}

	raw.resize (actually_read);
	bytes_read += actually_read;

	// Extract the trailing partial line of the already loaded content and
	// let it be completed by the new chunk
	std::string cur;
	if (!lines.empty ())
	{
		cur = lines.back ().raw_data;
		lines.pop_back ();
	}

	// The index of the first newly pushed line
	auto new_start = (int) lines.size ();

	for (auto c : raw)
	{
		if (c == '\n')
		{
			lines.push_back ({ cur });
			cur.clear ();
		}
		else if (c != '\r')
		{
			cur += c;
		}
	}

	lines.push_back ({ cur });

	// The already loaded lines keep their expanded width in longest_expanded;
	// only the new lines must be measured
	auto it = lines.begin ();
	std::advance (it, new_start);
	for (; it != lines.end (); ++it)
	{
		measure (*it);
	}

	// If we have read everything, finalize
	if (bytes_read + bom_offset >= total_size)
	{
		fully_loaded = true;
		CloseHandle (handle);
		handle = INVALID_HANDLE_VALUE;
	}
}

}
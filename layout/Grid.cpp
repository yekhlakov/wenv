#include <algorithm>
#include <unordered_map>
#include "Grid.h"
#include "../maxy/strings.h"

namespace Wenv::Layout {

void Grid::add_row (int min_size, int max_size, float percent_size)
{
	row_constraints.push_back ({min_size, max_size, percent_size});
}

void Grid::add_column (int min_size, int max_size, float percent_size)
{
	column_constraints.push_back ({min_size, max_size, percent_size});
}

std::vector<int> Grid::calculate_client_sizes
(
	const std::vector<Constraint> &constraints,
	int total_available_size,
	size_t count
) const
{
	std::vector<int> sizes (count, 0);

	if (count == 0 || total_available_size <= 0) 
	{
		return sizes;
	}

	// First pass: apply percentage constraints
	int remaining_size = total_available_size;
	int fixed_count = 0;

	for (auto i = 0; i < count; ++i) 
	{
		if (i < static_cast<int> (constraints.size ()) && constraints[i].percent_size > 0) 
		{
			sizes[i] = static_cast<int> (round ((double) total_available_size * constraints[i].percent_size / 100.0));

			// Apply min/max constraints
			if (i < static_cast<int> (constraints.size ())) 
			{
				if (constraints[i].min_client_size > 0) 
				{
					sizes[i] = std::max (sizes[i], constraints[i].min_client_size);
				}
				if (constraints[i].max_client_size > 0) 
				{
					sizes[i] = std::min (sizes[i], constraints[i].max_client_size);
				}
			}

			remaining_size -= sizes[i];
			fixed_count++;
		}
	}

	// Second pass: distribute remaining size among non-percentage columns/rows
	int flexible_count = (int) count - fixed_count;
	if (flexible_count <= 0 || remaining_size <= 0)
	{
		return sizes;
	}

	int equal_share = remaining_size / flexible_count;
	int remainder = remaining_size % flexible_count;

	int flex_idx = 0;
	for (size_t i = 0; i < count; ++i) 
	{
		if (i < constraints.size () && constraints[i].percent_size != 0)
		{
			continue;
		}

		sizes[i] = equal_share + (flex_idx < remainder ? 1 : 0);

		flex_idx++;

		// Apply min/max constraints
		if (i >= constraints.size ())
		{
			continue;
		}

		if (constraints[i].min_client_size > 0) 
		{
			sizes[i] = std::max (sizes[i], constraints[i].min_client_size);
		}
		if (constraints[i].max_client_size > 0) 
		{
			sizes[i] = std::min (sizes[i], constraints[i].max_client_size);
		}
	}

	return sizes;
}

Dimensions Grid::get_block_dimensions (Dimensions container_dimensions, Block grid_block_dimensions)
{
	Dimensions result;
	result.x = container_dimensions.x;
	result.y = container_dimensions.y;

	int start_col = grid_block_dimensions.x;
	int start_row = grid_block_dimensions.y;
	int num_cols = grid_block_dimensions.width;
	int num_rows = grid_block_dimensions.height;

	// Validate bounds
	if (start_col < 0 || start_row < 0 || num_cols <= 0 || num_rows <= 0)
	{
		result.width = 0;
		result.height = 0;

		return result;
	}

	if (start_col + num_cols > column_constraints.size() || start_row + num_rows > row_constraints.size())
	{
		result.width = 0;
		result.height = 0;

		return result;
	}

	// Calculate available space for content (subtracting border overhead)
	// In exclusive mode: each cell has 2 borders, so total border space = 2 * n_columns
	// In non-exclusive mode: there are (n_columns + 1) vertical border lines
	int available_width_for_content;
	int available_height_for_content;

	if (is_exclusive)
	{
		// Each column has its own complete border (left + right = 2 chars per column)
		available_width_for_content = container_dimensions.width - 2 * (int) column_constraints.size();
		available_height_for_content = container_dimensions.height - 2 * (int) row_constraints.size();
	}
	else
	{
		// Shared borders: (n_columns + 1) vertical lines, (n_rows + 1) horizontal lines
		available_width_for_content = container_dimensions.width - (int) (column_constraints.size() + 1);
		available_height_for_content = container_dimensions.height - (int) (column_constraints.size() + 1);
	}

	// Ensure we don't go negative
	available_width_for_content = std::max (0, available_width_for_content);
	available_height_for_content = std::max (0, available_height_for_content);

	// Calculate client sizes for all columns and rows
	std::vector<int> col_client_sizes = calculate_client_sizes
	(
		column_constraints, 
		available_width_for_content, 
		column_constraints.size()
	);
	std::vector<int> row_client_sizes = calculate_client_sizes 
	(
		row_constraints, 
		available_height_for_content, 
		row_constraints.size()
	);

	// Sum up the client sizes for all previous columns/rows
	for (int c = 0; c < start_col; c++)
	{
		result.x += col_client_sizes[c];
	}

	for (int r = 0; r < start_row; r++)
	{
		result.y += row_client_sizes[r];
	}

	// Sum up the client sizes for the block
	int block_client_width = 0;
	for (int c = start_col; c < start_col + num_cols; ++c) 
	{
		block_client_width += col_client_sizes[c];
	}
	int block_client_height = 0;
	for (int r = start_row; r < start_row + num_rows; ++r) 
	{
		block_client_height += row_client_sizes[r];
	}

	// Add border space for the block
	if (is_exclusive) 
	{
		result.x += start_col * 2;
		result.y += start_row * 2;

		// Each cell in the block has its own borders
		// For num_cols columns: each contributes 2 border chars (left + right)
		result.width = block_client_width + 2 * num_cols;
		result.height = block_client_height + 2 * num_rows;
	}
	else 
	{
		result.x += start_col;
		result.y += start_row;

		// Shared borders: (num_cols + 1) vertical border lines, (num_rows + 1) horizontal
		result.width = block_client_width + (num_cols + 1);
		result.height = block_client_height + (num_rows + 1);
	}

	if (result.x + result.width == container_dimensions.width - 1)
	{
		result.width++;
	}
	else if (result.x + result.width == container_dimensions.width + 1)
	{
		result.width--;
	}

	if (result.y + result.height == container_dimensions.height - 1)
	{
		result.height++;
	}
	else if (result.y + result.height == container_dimensions.height + 1)
	{
		result.height--;
	}

	return result;
}

void Grid::add_block (Dimensions grid_block_dimensions, int btype)
{
	blocks.push_back ({ grid_block_dimensions, btype });
}

void Grid::bake (Dimensions container_dimensions)
{
	for (auto &b : blocks)
	{
		b.container_dimensions = get_block_dimensions (container_dimensions, b);
	}

	// Char buffer to hold boundary texts
	std::vector<std::vector<wchar_t>> buffer (container_dimensions.height, std::vector<wchar_t> (container_dimensions.width + 1, L' '));

	auto u = [] (const char *p)->wchar_t { return maxy::strings::utf8towchar (p)[0]; };

	std::unordered_map<wchar_t, int> char_bitmasks =
	{
		{ u (" "), 0 },
		//		     rrbblltt
		{ u ("│"), 0b00010001 },
		{ u ("┤"), 0b00010101 },
		{ u ("╡"), 0b00011001 },
		{ u ("╢"), 0b00100110 },
		{ u ("╖"), 0b00100100 },
		{ u ("╕"), 0b00011000 },
		{ u ("╣"), 0b00101010 },
		{ u ("║"), 0b00100010 },
		//		     rrbblltt
		{ u ("╗"), 0b00101000 },
		{ u ("╝"), 0b00001010 },
		{ u ("╜"), 0b00000110 },
		{ u ("╛"), 0b00001001 },
		{ u ("┐"), 0b00010100 },
		{ u ("└"), 0b01000001 },
		{ u ("┴"), 0b01000101 },
		{ u ("┬"), 0b01010100 },
		{ u ("├"), 0b01010001 },
		{ u ("─"), 0b01000100 },
		{ u ("┼"), 0b01010101 },
		//		     rrbblltt
		{ u ("╞"), 0b10010001 },
		{ u ("╟"), 0b01100010 },
		{ u ("╚"), 0b10000010 },
		{ u ("╔"), 0b10100000 },
		{ u ("╩"), 0b10001010 },
		{ u ("╦"), 0b10101000 },
		{ u ("╠"), 0b10100010 },
		{ u ("═"), 0b10001000 },
		{ u ("╬"), 0b10101010 },
		//		     rrbblltt
		{ u ("╧"), 0b10001001 },
		{ u ("╨"), 0b01000110 },
		{ u ("╤"), 0b10011000 },
		{ u ("╥"), 0b01100100 },
		{ u ("╙"), 0b01000010 },
		{ u ("╘"), 0b10000001 },
		{ u ("╒"), 0b10010000 },
		{ u ("╓"), 0b01100000 },
		{ u ("╫"), 0b01100110 },
		{ u ("╪"), 0b10011001 },
		{ u ("┘"), 0b00000101 },
		{ u ("┌"), 0b01010000 }
	};

	std::unordered_map<int, wchar_t> bitmask_chars;

	for (auto &p : char_bitmasks)
	{
		bitmask_chars[p.second] = p.first;
	}

	auto mix_chars = [&] (wchar_t incoming, wchar_t previous)->wchar_t
	{
		auto in_mask = char_bitmasks[incoming];
		auto prev_mask = char_bitmasks[previous];
		auto new_mask = in_mask | prev_mask;

		if ((new_mask & 0b00000011) == 0b00000011)
		{
			new_mask &= 0b11111110;
		}
		if ((new_mask & 0b00001100) == 0b00001100)
		{
			new_mask &= 0b11111011;
		}
		if ((new_mask & 0b00110000) == 0b00110000)
		{
			new_mask &= 0b11101111;
		}
		if ((new_mask & 0b11000000) == 0b11000000)
		{
			new_mask &= 0b10111111;
		}


		if
		(
			//			  rrbblltt
			(new_mask & 0b00000011) == 0b00000010 &&
			(new_mask & 0b00110000)
		)
		{
			new_mask = (new_mask & 0b11001111) | 0b00100000;
		}

		if
			(
				//			  rrbblltt
				(new_mask & 0b00110000) == 0b00100000 &&
				(new_mask & 0b00000011)
			)
		{
			new_mask = (new_mask & 0b11111100) | 0b00000010;
		}

		if
		(
			//			  rrbblltt
			(new_mask & 0b00001100) == 0b00001000 && 
			(new_mask & 0b11000000)
		)
		{
			new_mask = (new_mask & 0b00111111) | 0b10000000;
		}

		if
			(
				//			  rrbblltt
				(new_mask & 0b11000000) == 0b10000000 &&
				(new_mask & 0b00001100)
			)
		{
			new_mask = (new_mask & 0b11110011) | 0b00001000;
		}

		return bitmask_chars[new_mask];
	};

	static const std::wstring element_base[][6] = {
		{
			maxy::strings::utf8towchar (" "),
			maxy::strings::utf8towchar (" "),
			maxy::strings::utf8towchar (" "),
			maxy::strings::utf8towchar (" "),
			maxy::strings::utf8towchar (" "),
			maxy::strings::utf8towchar (" ")
		},
		{
			maxy::strings::utf8towchar ("─"),
			maxy::strings::utf8towchar ("│"),
			maxy::strings::utf8towchar ("┌"),
			maxy::strings::utf8towchar ("┐"),
			maxy::strings::utf8towchar ("└"),
			maxy::strings::utf8towchar ("┘")
		},
		{
			maxy::strings::utf8towchar ("═"),
			maxy::strings::utf8towchar ("║"),
			maxy::strings::utf8towchar ("╔"),
			maxy::strings::utf8towchar ("╗"),
			maxy::strings::utf8towchar ("╚"),
			maxy::strings::utf8towchar ("╝")
		}
	};

	auto put_char = [&] (wchar_t ch, int x, int y)->void
	{
		auto prev = buffer[y][x];

		buffer[y][x] = mix_chars (ch, prev);
	};

	for (auto &b : blocks)
	{
		if (b.btype == 0)
		{
			// The block has no visible boundary
			continue;
		}

		// Corners
		put_char (element_base[b.btype][2][0], b.container_dimensions.x, b.container_dimensions.y);
		put_char (element_base[b.btype][3][0], b.container_dimensions.x + b.container_dimensions.width - 1, b.container_dimensions.y);
		put_char (element_base[b.btype][4][0], b.container_dimensions.x, b.container_dimensions.y + b.container_dimensions.height - 1);
		put_char (element_base[b.btype][5][0], b.container_dimensions.x + b.container_dimensions.width - 1, b.container_dimensions.y + b.container_dimensions.height - 1);
		
		// Edges
		for (auto x = 1; x < b.container_dimensions.width - 1; x++)
		{
			put_char (element_base[b.btype][0][0], b.container_dimensions.x + x, b.container_dimensions.y);
			put_char (element_base[b.btype][0][0], b.container_dimensions.x + x, b.container_dimensions.y + b.container_dimensions.height - 1);
		}

		for (auto y = 1; y < b.container_dimensions.height - 1; y++)
		{
			put_char (element_base[b.btype][1][0], b.container_dimensions.x, b.container_dimensions.y + y);
			put_char (element_base[b.btype][1][0], b.container_dimensions.x + b.container_dimensions.width - 1, b.container_dimensions.y + y);
		}
	}

	for (auto &b : blocks)
	{
		// Extract edge strings
		b.top_boundary = { &buffer[b.container_dimensions.y][b.container_dimensions.x], &buffer[b.container_dimensions.y][b.container_dimensions.x + b.container_dimensions.width] };
		b.bottom_boundary = { &buffer[b.container_dimensions.y + b.container_dimensions.height - 1][b.container_dimensions.x], &buffer[b.container_dimensions.y + b.container_dimensions.height - 1][b.container_dimensions.x + b.container_dimensions.width] };

		b.left_boundary = L"";
		b.right_boundary = L"";
		for (auto y = 1; y < b.container_dimensions.height - 1; y++)
		{
			b.left_boundary.push_back (buffer[b.container_dimensions.y + y][b.container_dimensions.x]);
			b.right_boundary.push_back (buffer[b.container_dimensions.y + y][b.container_dimensions.x + b.container_dimensions.width - 1]);
		}
	}
}

} // namespace Wenv::Layout

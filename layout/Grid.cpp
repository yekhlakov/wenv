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

// add one to the size of the last element in sizes that allows this subtraction
bool modify_size (Grid::Sizes &sizes, const std::vector<Grid::Constraint> &constraints, int addition)
{
	for (int p = (int) sizes.size () - 1; p >= 0; p--)
	{
		if (constraints[p].percent_size != 0)
		{
			// Skip percentage constraints
			continue;
		}

		auto tentative_size = sizes[p].second + addition;

		if (tentative_size < constraints[p].min_size || tentative_size > constraints[p].max_size)
		{
			// Cannot further modify this size 
			continue;
		}

		// We found a size that can be modified
		sizes[p].second = tentative_size;

		// Now shift all offsets that are after it
		for (int q = p + 1; q < (int) sizes.size (); q++)
		{
			sizes[q].first += addition;
		}

		return true;
	}

	// Could not find a flex size, so pick the last percentage size
	for (int p = (int) sizes.size () - 1; p >= 0; p--)
	{
		if (constraints[p].percent_size == 0)
		{
			// Skip flex constraints
			continue;
		}

		auto tentative_size = sizes[p].second + addition;

		if (tentative_size < 2)
		{
			// Cannot further modify this size 
			continue;
		}

		// We found a size that can be modified
		sizes[p].second = tentative_size;

		// Now shift all offsets that are after it
		for (int q = p + 1; q < (int) sizes.size (); q++)
		{
			sizes[q].first += addition;
		}

		return true;
	}

	// Could not find a size to modify AT ALL
	return false;
}

Grid::Sizes Grid::calculate_gross_sizes
(
	// List of constraints
	const std::vector<Constraint> &constraints,
	// Total container size (in characters)
	int total_available_size
) const
{
	auto count = constraints.size ();

	Sizes sizes (count, { 0, 0 });

	if (count == 0)
	{
		return sizes;
	}

	if (is_exclusive && total_available_size < count * 2)
	{
		return sizes;
	}

	if (!is_exclusive && total_available_size < count + 1)
	{
		return sizes;
	}

	// How many space is left for flex-size elements
	auto flex_size = total_available_size;

	// Iterate percent elements first
	size_t p = 0;
	size_t num_flex_elements = count;
	for (auto &cell : constraints)
	{
		if (cell.percent_size == 0.0)
		{
			p++;
			continue;
		}

		int actual_size = (int) round (total_available_size * 0.01 * cell.percent_size) + !is_exclusive;

		// respect fixed size constraints
		if (cell.min_size != 0)
		{
			actual_size = std::max (cell.min_size, actual_size);
		}
		if (cell.max_size != 0)
		{
			actual_size = std::min (cell.max_size, actual_size);
		}

		sizes[p].second = actual_size;
		flex_size -= actual_size;
		if (!is_exclusive)
		{
			flex_size ++;
		}
		num_flex_elements--;

		p++;
	}

	if (num_flex_elements)
	{
		auto default_flex_percentage = 1. / num_flex_elements;
		// Iterate flexible elements
		size_t p = 0;
		for (auto &cell : constraints)
		{
			if (cell.percent_size != 0.0)
			{
				p++;
				continue;
			}

			int actual_size = (int) round (flex_size * default_flex_percentage) + !is_exclusive;

			// respect fixed size constraints
			if (cell.min_size != 0)
			{
				actual_size = std::max (cell.min_size, actual_size);
			}
			if (cell.max_size != 0)
			{
				actual_size = std::min (cell.max_size, actual_size);
			}

			sizes[p].second = actual_size;

			p++;
		}
	}

	// Now compute offsets
	auto sum = 0;
	for (auto &cell : sizes)
	{
		cell.first = sum;

		sum += cell.second;
		if (!is_exclusive)
		{
			sum--;
		}
	}

	if (!is_exclusive)
	{
		sum++;
	}

	// Adjust the last element size to fit the whole container (to fix rounding errors)
	
	while (sum < total_available_size)
	{
		if (!modify_size (sizes, constraints, +1))
		{
			break;
		}

		sum++;
	}
	
	while (sum > total_available_size)
	{
		if (!modify_size (sizes, constraints, -1))
		{
			break;
		}

		sum--;
	}
	

	return sizes;
}

// Calculate dimensions for given block using gross sizes of underlying rows and cols
Dimensions Grid::calculate_block_dimensions (
	Dimensions container_dimensions,
	Block grid_block_dimensions,
	const std::vector<std::pair<int, int>> &gross_row_sizes,
	const std::vector<std::pair<int, int>> &gross_col_sizes
)
{
	Dimensions result;

	result.x = container_dimensions.x + gross_col_sizes[grid_block_dimensions.x].first;
	result.y = container_dimensions.y + gross_row_sizes[grid_block_dimensions.y].first;

	auto endx = container_dimensions.x
		+ gross_col_sizes[grid_block_dimensions.x + grid_block_dimensions.width - 1].first
		+ gross_col_sizes[grid_block_dimensions.x + grid_block_dimensions.width - 1].second;

	auto endy = container_dimensions.y
		+ gross_row_sizes[grid_block_dimensions.y + grid_block_dimensions.height - 1].first
		+ gross_row_sizes[grid_block_dimensions.y + grid_block_dimensions.height - 1].second;

	result.width = endx - result.x;
	result.height = endy - result.y;

	return result;
}

void Grid::add_block (Dimensions grid_block_dimensions, int btype, Grid * nested_grid, ::Wenv::Apps::App * app)
{
	blocks.push_back ({ grid_block_dimensions, btype, nested_grid, app });
}

void Grid::bake (Dimensions container_dimensions, std::vector<std::vector<wchar_t>> & buffer)
{
	auto cols = calculate_gross_sizes (column_constraints, (int) container_dimensions.width);
	auto rows = calculate_gross_sizes (row_constraints, (int) container_dimensions.height);

	for (auto &b : blocks)
	{
		b.container_dimensions = calculate_block_dimensions (container_dimensions, b, rows, cols);
	}

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

	// Process nested grids
	for (auto &b : blocks)
	{
		if (b.grid != nullptr)
		{
			b.grid->bake (b.container_dimensions, buffer);
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

Dimensions Block::get_client_dimensions () const
{
	if (btype < 0)
	{
		return container_dimensions;
	}

	return Dimensions
	{
		container_dimensions.x + 1,
		container_dimensions.y + 1,
		container_dimensions.x + container_dimensions.width - 2,
		container_dimensions.y + container_dimensions.height - 2,
	};
}


} // namespace Wenv::Layout

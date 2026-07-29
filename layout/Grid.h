#pragma once
#include <string>
#include <vector>
#include "../Types.h"

namespace Wenv::Apps {
class App;
class Context;
}

namespace Wenv::Layout {

using Dimensions = ::Wenv::Display::Rect;

struct Grid;

// The visible cell of the grid. A block occupies a rectangle of n*m contiguous cells
struct Block : public Dimensions
{
	// -1 - borderless block (special case available in EXCLUSIVE grids only)
	// 0 - empty boundary (made of whitespace)
	// 1 - single-line boundary
	// 2 = double line boundary
	int btype = 1;

	// Nested grid
	Grid *grid = nullptr;

	// Attached app
	::Wenv::Apps::App *app = nullptr;

	Dimensions container_dimensions;

	std::wstring top_boundary;
	std::wstring left_boundary;
	std::wstring right_boundary;
	std::wstring bottom_boundary;

	Dimensions get_client_dimensions () const;
};

struct Grid
{
	using Sizes = std::vector<std::pair<int, int>>;

	struct Constraint
	{
		int min_size;
		int max_size;
		float percent_size;
	};

	std::vector<Constraint> row_constraints;
	std::vector<Constraint> column_constraints;
	std::vector<Block> blocks;

	std::vector<std::vector<wchar_t>> boundary_elements;

	// The context for all apps in this grid
	::Wenv::Apps::Context *context = nullptr;

	// Is the grid exclusive (== do the blocks have their own `exclusive` boundary or they share their boundaries)
	bool is_exclusive;

	// Add underlying grid row
	void add_row (int min_size, int max_size, float percent_size);
	// Add underlying grid column
	void add_column (int min_size, int max_size, float percent_size);
	// Add grid block (possibly occupying more than one underlying cell)
	void add_block (Dimensions grid_block_dimensions, int btype, Grid *nested_grid = nullptr, ::Wenv::Apps::App * app = nullptr);
	// Bake the grid (compute dimensions and boundary strings for all its blocks)
	void bake (Dimensions container_dimensions, std::vector<std::vector<wchar_t>> & buffer);


	// Calculate gross sizes for each underlying cell
	// return pairs {start_pos, size} for each cell
	Sizes calculate_gross_sizes
	(
		const std::vector<Constraint> &constraints,
		int total_available_size
	) const;

	// Calculate dimensions for given block using gross sizes of underlying rows and cols
	Dimensions calculate_block_dimensions (
		Dimensions container_dimensions, 
		Block grid_block_dimensions,
		const std::vector<std::pair<int, int>> & gross_row_sizes,
		const std::vector<std::pair<int, int>> & gross_col_sizes
	);
};

} // namespace Wenv::Layout

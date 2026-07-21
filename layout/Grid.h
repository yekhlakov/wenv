#pragma once
#include <string>
#include <vector>
#include "Types.h"

namespace Wenv::Layout {

using Dimensions = ::Wenv::Display::Rect;

// The visible cell of the grid. A block occupies a rectangle of n*m contiguous cells
struct Block : public Dimensions
{
	// -1 - borderless block (special case available in EXCLUSIVE grids only)
	// 0 - empty boundary (made of whitespace)
	// 1 - single-line boundary
	// 2 = double line boundary
	int btype;

	Dimensions container_dimensions;

	std::wstring top_boundary;
	std::wstring left_boundary;
	std::wstring right_boundary;
	std::wstring bottom_boundary;
};

struct Grid
{
	struct Constraint
	{
		int min_client_size;
		int max_client_size;
		float percent_size;
	};

	std::vector<Constraint> row_constraints;
	std::vector<Constraint> column_constraints;
	std::vector<Block> blocks;

	std::vector<std::vector<wchar_t>> boundary_elements;

	// Is the grid exclusive (== do the blocks have their own `exclusive` boundary or they share their boundaries)
	bool is_exclusive;

	// Add underlying grid row
	void add_row (int min_size, int max_size, float percent_size);
	// Add underlying grid column
	void add_column (int min_size, int max_size, float percent_size);
	// Add grid block (possibly occupying more than one underlying cell)
	void add_block (Dimensions grid_block_dimensions, int btype);
	// Bake the grid (compute dimensions and boundary strings for all its blocks)
	void bake (Dimensions container_dimensions);

	// Helper to compute block dimensions
	Dimensions get_block_dimensions (Dimensions container_dimensions, Block grid_block_dimensions);

	// Helper to calculate client sizes for all underlying rows and columns
	std::vector<int> calculate_client_sizes
	(
		const std::vector<Constraint> &constraints,
		int total_available_size,
		size_t count
	) const;
};

} // namespace Wenv::Layout

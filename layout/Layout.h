#pragma once
#include <vector>
#include "Types.h"

namespace Wenv::Layout {

using Dimensions = ::Wenv::Display::Rect;

struct Grid
{
	// The grid always occupies the whole area it is given
	struct Constraint
	{
		// Constraints on size (in characters)
		// 0 means no constraint
		int min_client_size;
		int max_client_size;

		// Percentage constraint on size
		// 0 means no percentage constraint (the size may change freely)
		float percent_size;
	};

	// Number of columns and rows in a grid
	int n_rows;
	int n_columns;

	std::vector<Constraint> row_constraints;
	std::vector<Constraint> column_constraints;

	// if true, the cells do not overlap (== each cell contains its own border and does not share border with other cells)
	// if false, cells may overlap (== shared borders)
	bool is_exclusive;

	std::vector<int> calculate_client_sizes (
		const std::vector<Constraint> &constraints,
		int total_available_size,
		int count) const;

	// Get the dimensions (in charaters) of a block of cells in the grid.
	// The block dimensions always contain its borders (always 1 character wide)
	// The grid block may contain more than one cell.
	// The grid block dimensions are given in cells, not in characters.
	Dimensions get_block_dimensions (Dimensions contanier_dimensions, Dimensions grid_block_dimensions);
};

// The layout consists of frames
// Each frame has its own boundary (or perhaps no boundary)
// Top level frames cannot share boundaries.
// (On the contrary, second level frames do have only shared boundaries)
struct Layout
{
	// The layout box. May or may not have its own boundary
	struct Box
	{
		int top_boundary_type; // 0 = none, 1 = single line, 2 = double line
		int left_boundary_type;
		int bottom_boundary_type;
		int right_boundary_type;

		// Calculated position and size of the box INCLUDING THE BOUNDARY [in characters]
		Dimensions outer_dimensions;

		// Calculated dimensions of the client area of the box [in characters]
		// The client area may or may not differ from the whole box depending on its boundary type
		Dimensions client_dimensions;

		// Where in its parent's grid this box is located [IN GRID CELLS!]
		Dimensions grid_dimensions;
	};

	// Terminal box. Contains the content produced by an applet.
	struct Container : public Box
	{
		// Should the content be clipped to container client area
		bool is_clipped;

		// Is the content scrollable
		bool has_vscroll;
		bool has_hscroll;

		// Applet info
		// The applet provides:
		// - content
		// - title text (written on the top boundary)
		// - status bar text (written on the bottom boundary)
		// 
	};

	// Top level frame derives from box
	struct Frame : public Box
	{
		Grid grid;

		// The containers defined within the frame.
		std::vector<Container> containers;
	};

	// All frames of a layout are arranged on a grid NxM
	// (All containers of a frame are arranged on a grid too)
	// A frame may span any number of grid elements (but these must be contiguous and form a rectangle)
	Grid grid;

	std::vector<Frame> frames;
};

} // namespace Wenv::Layout

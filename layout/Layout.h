#pragma once
#include "Grid.h"

namespace Wenv::Layout {

using Dimensions = ::Wenv::Display::Rect;


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
};

// Top level frame derives from box
struct Frame : public Box
{
	Grid grid;

	// The containers defined within the frame.
	std::vector<Container> containers;
};



// The layout consists of frames
// Each frame has its own boundary (or perhaps no boundary)
// Top level frames cannot share boundaries.
// (On the contrary, second level frames do have shared boundaries)
struct Layout
{
	// Root frame
	Frame	frame;
};

} // namespace Wenv::Layout


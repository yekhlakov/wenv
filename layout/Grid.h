#pragma once
#include <vector>
#include "Types.h"

namespace Wenv::Layout {

using Dimensions = ::Wenv::Display::Rect;

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

	bool is_exclusive;

	void add_row (int min_size, int max_size, float percent_size);
	void add_column (int min_size, int max_size, float percent_size);

	std::vector<int> calculate_client_sizes (
		const std::vector<Constraint> &constraints,
		int total_available_size,
		int count) const;

	Dimensions get_block_dimensions (Dimensions container_dimensions, Dimensions grid_block_dimensions);
};

} // namespace Wenv::Layout

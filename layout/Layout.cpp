#include <algorithm>
#include "Layout.h"

namespace Wenv::Layout {

// Helper method to calculate client sizes for rows or columns
std::vector<int> Grid::calculate_client_sizes
(
    const std::vector<Constraint> &constraints,
    int total_available_size,
    int count
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

    for (int i = 0; i < count; ++i) 
    {
        if (i < static_cast<int> (constraints.size ()) && constraints[i].percent_size > 0) 
        {
            sizes[i] = static_cast<int> (total_available_size * constraints[i].percent_size / 100.0f);

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
    int flexible_count = count - fixed_count;
    if (flexible_count > 0 && remaining_size > 0) 
    {
        int equal_share = remaining_size / flexible_count;
        int remainder = remaining_size % flexible_count;

        int flex_idx = 0;
        for (int i = 0; i < count; ++i) 
        {
            if (i >= static_cast<int> (constraints.size ()) || constraints[i].percent_size == 0) 
            {
                sizes[i] = equal_share + (flex_idx < remainder ? 1 : 0);

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

                flex_idx++;
            }
        }
    }

    return sizes;
}

// Get the dimensions (in characters) of a block of cells in the grid.
// The block dimensions always contain its borders (always 1 character wide)
// The grid block may contain more than one cell.
// The grid block dimensions are given in cells, not in characters.
Dimensions Grid::get_block_dimensions (Dimensions container_dimensions, Dimensions grid_block_dimensions)
{
    Dimensions result;
    result.x = grid_block_dimensions.x;
    result.y = grid_block_dimensions.y;

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

    if (start_col + num_cols > n_columns || start_row + num_rows > n_rows)
    {
        result.width = 0;
        result.height = 0;
        return result;
    }

    // Calculate available space for content (subtracting border overhead)
    // For the entire grid:
    // In exclusive mode: each of n_columns has 2 borders, so total border space = 2 * n_columns
    // In non-exclusive mode: there are (n_columns + 1) vertical border lines

    int available_width_for_content;
    int available_height_for_content;

    if (is_exclusive)
    {
        // Each column has its own complete border (left + right = 2 chars per column)
        available_width_for_content = container_dimensions.width - 2 * n_columns;
        available_height_for_content = container_dimensions.height - 2 * n_rows;
    }
    else
    {
        // Shared borders: (n_columns + 1) vertical lines, (n_rows + 1) horizontal lines
        available_width_for_content = container_dimensions.width - (n_columns + 1);
        available_height_for_content = container_dimensions.height - (n_rows + 1);
    }

    // Ensure we don't go negative
    available_width_for_content = std::max (0, available_width_for_content);
    available_height_for_content = std::max (0, available_height_for_content);

    // Calculate client sizes for all columns and rows
    std::vector<int> col_client_sizes = calculate_client_sizes
    (
        column_constraints, 
        available_width_for_content, 
        n_columns
    );
    std::vector<int> row_client_sizes = calculate_client_sizes 
    (
        row_constraints, 
        available_height_for_content, 
        n_rows
    );

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
        // Each cell in the block has its own borders
        // For num_cols columns: each contributes 2 border chars (left + right)
        result.width = block_client_width + 2 * num_cols;
        result.height = block_client_height + 2 * num_rows;
    }
    else 
    {
        // Shared borders: (num_cols + 1) vertical border lines, (num_rows + 1) horizontal
        result.width = block_client_width + (num_cols + 1);
        result.height = block_client_height + (num_rows + 1);
    }

    return result;
}

}

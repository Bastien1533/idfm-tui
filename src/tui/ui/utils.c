#include "tui/ui/button.h"
#include "tui/ui/input.h"

#include <stdbool.h>
#include <stddef.h>

bool point_in_input(int x, int y, ui_input *input)
{
        if (input == NULL)
                return false;

        int plane_y, plane_x;
        unsigned rows, cols;

        struct ncplane *plane = ncreader_plane(input->reader);

        ncplane_abs_yx(plane, &plane_y, &plane_x);
        ncplane_dim_yx(plane, &rows, &cols);

        return (x >= plane_x && x < plane_x + (int)cols) &&
               (y >= plane_y && y < plane_y + (int)rows);
}

bool point_in_btn(int x, int y, ui_button *btn)
{
        if (btn == NULL || btn->opts == NULL)
                return false;

        int plane_y, plane_x;
        unsigned rows, cols;

        ncplane_abs_yx(btn->plane, &plane_y, &plane_x);
        ncplane_dim_yx(btn->plane, &rows, &cols);

        return (x >= plane_x && x < plane_x + (int)cols) &&
               (y >= plane_y && y < plane_y + (int)rows);
}

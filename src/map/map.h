#ifndef MAP_H
#define MAP_H

#include <stddef.h>
#include <stdint.h>

struct geo_pt {
        double lat;
        double lon;
};

struct flat_pt {
        double x;
        double y;
};

struct cell_pos {
        int row;
        int col;
};

struct map_cell {
        uint32_t fg;
        uint32_t bg;
        uint32_t ch;
        double min_scale;
        double max_scale;
};

struct bbox {
        double xmax;
        double ymax;

        double xmin;
        double ymin;
};

struct map_view {
        double cy;
        double cx;

        size_t rows;
        size_t cols;

        size_t u_rows;
        size_t u_cols;

        double aspect;
        double scale;
        double lat_ref;

        struct bbox view_box;
};

void map_render_loop(void);

void map_fit(struct map_view *view, struct bbox bbox);
struct map_view map_init(size_t rows, size_t cols, double lat_ref);

void map_print_frame(struct map_view *view, struct map_cell *cells,
                     size_t n_cells);
void map_move_view(struct map_view *view, int off_x, int off_y);
void map_clear_frame(struct map_view *view);

#endif

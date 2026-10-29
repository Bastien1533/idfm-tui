#ifndef MAP_H
#define MAP_H

#include <stddef.h>
#include <stdint.h>
#include <notcurses/notcurses.h>

typedef struct worker_data worker_data;

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

        double origin_x;
        double origin_y;

        size_t rows;
        size_t cols;

        size_t u_rows;
        size_t u_cols;

        double aspect;
        double scale;
        double lat_ref;

        struct bbox view_box;
};

typedef struct flat_shape flat_shape;
struct shape;
struct stop;

void map_render_loop(void);

void map_fit(struct map_view *view, struct bbox bbox);
void map_fit_shapes(struct map_view *view, struct flat_shape *shapes,
                    size_t n_shapes);
void draw_map(size_t n_shapes, struct flat_shape *flat_shapes,
              struct map_view view, struct map_cell **cells,
              struct ncplane *base_plane);
struct map_view map_init(size_t rows, size_t cols, double lat_ref,
                         double origin_x, double origin_y);
void handle_map_input(const ncinput *ni, uint32_t key, struct map_view *view);

void map_print_frame(struct map_view *view, struct map_cell *cells,
                     size_t n_cells);
void map_move_view(struct map_view *view, int off_x, int off_y);
void map_clear_frame(struct map_view *view);

#endif

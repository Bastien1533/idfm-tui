#ifndef MAP_UI_H
#define MAP_UI_H

#include "map/map.h"
#include "map/shapes.h"
#include <notcurses/notcurses.h>
#include <stdarg.h>
#include <stddef.h>

typedef struct scene scene_t;
typedef struct ui_map ui_map;

typedef struct {
        int x;
        int y;
        size_t cols;
        size_t rows;
        double lat_ref;

        struct flat_shape *shapes;
        size_t n_shapes;

        void (*handle_input)(ui_map *self, const ncinput *ni, uint32_t key);

        void (*onclick)(void *arg);
        void *onclick_payload;
} ui_map_opts;

typedef struct ui_map {
        struct ncplane *plane;
        struct ncplane *parent;
        struct map_cell *cells;
        struct map_view view;
        ui_map_opts *opts;
        struct notcurses *nc;

} ui_map;

void ui_resize_map(ui_map *map, size_t h, size_t w); // cols and rows
void ui_move_map(ui_map *map, int off_x,
                 int off_y); // off_x and off_y are cells offsets
void ui_map_fit(ui_map *map, struct bbox bbox); // lat/lon bbox
void ui_map_fit_shapes(ui_map *map);
ui_map *create_map(struct ncplane *parent, ui_map_opts *opts,
                   struct notcurses *nc);

void ui_draw_map(ui_map *map);
void free_map(ui_map *map);
#endif

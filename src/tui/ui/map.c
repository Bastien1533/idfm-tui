#include "tui/ui/map.h"
#include "map/map.h"
#include <notcurses/notcurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void ui_draw_map(ui_map *map)
{
        draw_map(map->opts->n_shapes, map->opts->shapes, map->view, &map->cells,
                 map->plane);
        notcurses_render(map->nc);
        memset(map->cells, 0,
               (size_t)map->opts->rows * map->opts->cols *
                       sizeof(struct map_cell));
}

void ui_resize_map(ui_map *map, size_t h, size_t w)
{
        map->opts->cols = w;
        map->opts->rows = h;
        ui_draw_map(map);
        return;
}

void ui_move_map(ui_map *map, int off_x, int off_y)
{
        struct map_view view = map->view;
        map_move_view(&view, off_x, off_y);
        map->view = view;
        ui_draw_map(map);
}
void ui_map_fit(ui_map *map, struct bbox bbox)
{
        struct map_view view = map->view;
        map_fit(&view, bbox);
        map->view = view;
        ui_draw_map(map);
}
void ui_map_fit_shapes(ui_map *map)
{
        struct map_view view = map->view;
        map_fit_shapes(&view, map->opts->shapes, map->opts->n_shapes);
        map->view = view;
        ui_draw_map(map);
}

ui_map *create_map(struct ncplane *parent, ui_map_opts *opts,
                   struct notcurses *nc)
{
        if (opts == NULL)
                return NULL;
        struct ncplane_options nopts = {
                .x = opts->x,
                .y = opts->y,
                .rows = opts->rows, // Height (lines)
                .cols = opts->cols, // Width (columns)
                .userptr = NULL, // Optional user data pointer
                .name = "map" // Optional string label for debugging
        };
        struct ncplane *map_plane = ncplane_create(parent, &nopts);

        if (!map_plane)
                return NULL;

        ui_map *map = calloc(1, sizeof(ui_map));
        ui_map_opts *heap_opts = malloc(sizeof(ui_map_opts));
        if (!map || !heap_opts) {
                free(map);
                free(heap_opts);
                ncplane_destroy(map_plane);
                return NULL;
        }
        *heap_opts = *opts;
        map->opts = heap_opts;
        map->plane = map_plane;
        map->parent = parent;
        map->cells = calloc(opts->rows * opts->cols, sizeof(struct map_cell));
        map->nc = nc;

        struct map_view view = map_init(opts->rows, opts->cols, opts->lat_ref);
        map->view = view;

        ui_map_fit_shapes(map);

        return map;
}

void free_map(ui_map *map)
{
        if (map == NULL)
                return;
        if (map->opts != NULL) {
                free(map->opts);
        }
        if (map->cells != NULL)
                free(map->cells);
        ncplane_destroy(map->plane);
        free(map);
}

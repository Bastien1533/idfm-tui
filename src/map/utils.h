#ifndef MAP_UTILS_H
#define MAP_UTILS_H

#include "map/shapes.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "map/map.h"

typedef struct flat_shape flat_shape;

bool paint_flat_line(struct flat_pt *points, size_t n_points,
                     struct map_view *map_view, struct map_cell *cells,
                     size_t *n_painted, uint32_t color);
bool paint_flat_shape(struct flat_shape shape, struct map_view *map_view,
                      struct map_cell *cells, size_t *n_painted);

bool paint_horiz_line(struct flat_pt start, struct flat_pt end,
                      size_t orientation, struct map_view *map_view,
                      struct map_cell *cells, size_t *n_painted);

bool shape_valid(const struct flat_shape *s);

#endif

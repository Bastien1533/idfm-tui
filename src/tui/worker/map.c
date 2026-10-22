#include "map/map.h"
#include "tui/protocol.h"
#include "data/gtfs.h"

#include <notcurses/notcurses.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct shape_res *get_flat_shapes(struct shape **shapes, size_t n_shapes,
                                  struct stop **stops, size_t n_stops)
{
        size_t n_flat_shapes = 0;
        double lat_ref = 52.5173885;

        struct flat_shape *flat_shapes = init_flat_shapes(
                shapes, n_shapes, &n_flat_shapes, stops, n_stops, lat_ref);
        if (flat_shapes == NULL)
                return NULL;
        struct shape_res *res = malloc(1 * sizeof(struct shape_res));
        if (res == NULL)
                return NULL;
        res->n_shapes = n_flat_shapes;
        res->shapes = flat_shapes;
        return res;
}

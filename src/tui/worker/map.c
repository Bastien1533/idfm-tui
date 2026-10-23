#include "map/map.h"
#include "tui/protocol.h"
#include "data/gtfs.h"

#include "utils/hashmap.h"
#include <notcurses/notcurses.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct shape_res *get_flat_shapes(struct trip **trips, size_t n_trips,
                                  size_t n_shapes, struct hash_map *shape_map,
                                  size_t n_stops)
{
        double lat_ref = 52.5173885;

        size_t n_flat_shapes = 0;
        struct flat_shape *flat_shapes =
                flat_shapes_from_trips(trips, n_trips, shape_map, n_shapes,
                                       &n_flat_shapes, n_stops, lat_ref);
        if (flat_shapes == NULL)
                return NULL;
        struct shape_res *res = malloc(1 * sizeof(struct shape_res));
        if (res == NULL)
                return NULL;
        res->n_shapes = n_flat_shapes;
        res->shapes = flat_shapes;
        return res;
}

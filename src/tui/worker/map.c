#include "map/map.h"
#include "tui/protocol.h"
#include "data/gtfs.h"

#include "utils/hashmap.h"
#include <notcurses/notcurses.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static size_t filter_trips(struct trip **trips, size_t n_trips,
                           struct trip **dst, logic_task_shape_req req)
{
        size_t new_n_trips = 0;
        struct hash_map *excluded_trips = init_hashmap(req.n_exclude_trips);
        struct hash_map *included_trips = init_hashmap(req.n_include_trips);

        for (size_t i = 0; i < req.n_include_trips; i++)
                hash_map_insert(included_trips, req.include_trips[i]->trip_id,
                                req.include_trips[i]);
        for (size_t i = 0; i < req.n_exclude_trips; i++)
                hash_map_insert(excluded_trips, req.exclude_trips[i]->trip_id,
                                req.exclude_trips[i]);

        for (size_t i = 0; i < n_trips; i++) {
                struct trip *t = trips[i];

                if (hash_map_get(included_trips, t->trip_id) != NULL &&
                    hash_map_get(excluded_trips, t->trip_id) == NULL)
                        dst[new_n_trips++] = t;

                if (req.n_include_trips == 0 &&
                    hash_map_get(excluded_trips, t->trip_id) == NULL)
                        dst[new_n_trips++] = t;
        }
        return new_n_trips;
}

struct shape_res *get_flat_shapes(struct trip **trips, size_t n_trips,
                                  size_t n_shapes, struct hash_map *shape_map,
                                  size_t n_stops,logic_task_shape_req req)
{
        struct trip **filtered_trips = calloc(n_trips, sizeof(struct trip *));
        size_t new_n_trips = filter_trips(trips, n_trips, filtered_trips, req);
        double lat_ref = 52.5173885;

        size_t n_flat_shapes = 0;
        struct flat_shape *flat_shapes = flat_shapes_from_trips(
                filtered_trips, new_n_trips, shape_map, n_shapes,
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

struct shape_res *get_itinerary_flat_shapes(struct trip **trips, size_t n_trips,
                                  size_t n_shapes, struct hash_map *shape_map,
                                  size_t n_stops, logic_task_itinerary_shape_req req)
{
        struct trip **filtered_trips = calloc(n_trips, sizeof(struct trip *));
        logic_task_shape_req filter_req = {
            .n_exclude_trips = 0,
            .n_include_trips = req.n_trips,
            .include_trips = req.trips,
        };
        size_t new_n_trips = filter_trips(trips, n_trips, filtered_trips, filter_req);
        double lat_ref = 52.5173885;

        size_t n_flat_shapes = 0;
        struct flat_shape *flat_shapes = flat_shapes_from_trips(
                filtered_trips, new_n_trips, shape_map, n_shapes,
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

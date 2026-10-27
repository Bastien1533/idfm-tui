#include "map/map.h"
#include "map/trip_cut.h"
#include "map/shapes.h"
#include "algo/raptor.h"
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
                                  size_t n_stops, double lat_ref,
                                  logic_task_shape_req req)
{
        struct trip **filtered_trips = calloc(n_trips, sizeof(struct trip *));
        size_t new_n_trips = filter_trips(trips, n_trips, filtered_trips, req);

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
        res->lat_ref = lat_ref;
        return res;
}

struct shape_res *get_itinerary_flat_shapes(struct raptor_itinerary *itinerary,
                                            size_t n_shapes,
                                            struct hash_map *shape_map,
                                            size_t n_stops, double lat_ref)
{
        struct trip **filtered_trips =
                calloc(itinerary->n_trips, sizeof(struct trip *));

        size_t trip_idx = 0;
        for (size_t i = 0; i < itinerary->n_legs; i++) {
                struct raptor_leg *leg = itinerary->legs[i];
                if (leg->trip != NULL) {
                        filtered_trips[trip_idx++] = leg->trip;
                }
        }

        size_t n_flat_shapes = 0;
        struct flat_shape *flat_shapes =
                flat_shapes_from_itinerary(itinerary, shape_map, n_shapes,
                                           &n_flat_shapes, n_stops, lat_ref);

        if (flat_shapes == NULL)
                return NULL;
        struct shape_res *res = malloc(1 * sizeof(struct shape_res));
        if (res == NULL)
                return NULL;
        res->n_shapes = n_flat_shapes;
        res->shapes = flat_shapes;
        res->lat_ref = lat_ref;
        return res;
}

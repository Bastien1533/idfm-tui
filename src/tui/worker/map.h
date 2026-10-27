#ifndef TUI_WORKER_MAP_H
#define TUI_WORKER_MAP_H

#include "tui/protocol.h"
#include "data/gtfs.h"
#include "utils/hashmap.h"
#include <notcurses/notcurses.h>

struct shape_res *get_flat_shapes(struct trip **trips, size_t n_trips,
                                  size_t n_shapes, struct hash_map *shape_map,
                                  size_t n_stops, double lat_ref,
                                  logic_task_shape_req req);

struct shape_res *get_itinerary_flat_shapes(struct raptor_itinerary* itinerary,
                                            size_t n_shapes,
                                            struct hash_map *shape_map,
                                            size_t n_stops, double lat_ref,
                                            logic_task_itinerary_shape_req req);

#endif

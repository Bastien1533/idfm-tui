#ifndef TUI_WORKER_MAP_H
#define TUI_WORKER_MAP_H

#include "tui/protocol.h"
#include "data/gtfs.h"
#include <notcurses/notcurses.h>

struct shape_res *get_flat_shapes(struct trip **trips, size_t n_trips,
                                  size_t n_shapes, struct hash_map *shape_map,
                                  size_t n_stops);

#endif

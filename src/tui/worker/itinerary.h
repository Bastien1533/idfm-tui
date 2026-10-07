#ifndef TUI_WORKER_ITINERARY_H
#define TUI_WORKER_ITINERARY_H

#include "tui/protocol.h"
#include "data/gtfs.h"
#include "algo/raptor.h"
#include "utils/hashmap.h"
#include <notcurses/notcurses.h>

struct itinerary_res *get_itinerary(logic_task_itinerary_req *request,
                                    struct raptor_stop **raptor_stops,
                                    size_t n_raptor_stops,
                                    size_t n_raptor_routes,
                                    struct hash_map *raptor_stop_map);

#endif

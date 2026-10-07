#ifndef TUI_WORKER_SEARCH_H
#define TUI_WORKER_SEARCH_H

#include "tui/protocol.h"
#include "data/gtfs.h"
#include <notcurses/notcurses.h>

struct search_res *get_search_stops(logic_task_stop_req *request,
                                    struct stop **stops, size_t n_stops);

#endif

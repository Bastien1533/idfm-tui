#ifndef TUI_WORKER_SEARCH_H
#define TUI_WORKER_SEARCH_H

#include "tui/protocol.h"
#include "data/gtfs.h"
#include <notcurses/notcurses.h>

struct matching_item {
        int dist;
        struct ncselector_item *item;
        void *orig_item;
};

struct matching_item **perform_stop_search(logic_task_stop_req *request,
                                           struct stop **stops, size_t n_stops,
                                           size_t *out_match_n);
search_res *format_stop_search_results(struct matching_item **matching_items,
                                       size_t match_n, size_t n_stops);

#endif

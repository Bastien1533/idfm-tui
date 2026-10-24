#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "algo/raptor.h"
#include "map/shapes.h"
#include <stddef.h>
#include <notcurses/notcurses.h>

typedef enum {
        UI_MSG_LOG,
        UI_INIT_FINISHED,
        UI_MSG_ITINERARY,
        UI_ERASE,
        UI_STOP_RESULT_DATA,
        UI_ITINERARY_RESULT_DATA,
        UI_SHAPE_RESULT_DATA,
} ui_message_type;

typedef struct {
        char *text;
} ui_message_log;

typedef struct search_res {
        size_t n_res;
        struct ncselector_item **items;
} search_res;

typedef struct shape_res {
        size_t n_shapes;
        struct flat_shape *shapes;
} shape_res;

typedef struct itinerary_res {
        size_t n_itineraries;
        struct raptor_itinerary **itineraries;
        struct ncselector_item **search_items;
} itinerary_res;

typedef struct ui_message {
        ui_message_type type;
        size_t seq;
        union {
                struct search_res *res;
                struct itinerary_res *itinerary_req_res;
                ui_message_log log;
                struct raptor_itinerary *itinerary;
                struct shape_res *shapes;
        } as;
} ui_message;

typedef enum {
        LOGIC_TASK_INIT_REQ,
        LOGIC_TASK_STOP_REQ,
        LOGIC_TASK_ITINERARY_REQ,
        LOGIC_TASK_SHAPES_REQ,
        LOGIC_TASK_ITINERARY_SHAPES_REQ,
        LOGIC_SHUTDOWN,
} logic_task_type;

typedef struct logic_task_itinerary_req {
        char *dep_id;
        char *arr_id;
} logic_task_itinerary_req;

typedef struct logic_task_stop_req {
        char *route_name;
        char *stop_name;
} logic_task_stop_req;

typedef struct logic_task_shape_req {
        struct trip **include_trips;
        size_t n_include_trips;
        struct trip** exclude_trips;
        size_t n_exclude_trips;
} logic_task_shape_req;

typedef struct logic_task_itinerary_shape_req {
    struct trip** trips;
    size_t n_trips;
    struct stop* dep;
    struct stop* arr;
} logic_task_itinerary_shape_req;

typedef struct {
        logic_task_type type;
        size_t seq;
        union {
                logic_task_shape_req shape_req;
                logic_task_itinerary_shape_req itinerary_shape_req;
                logic_task_itinerary_req itinerary_req;
                logic_task_stop_req stop_req;
        };
} logic_task_req;

void free_search_res(search_res *res);

#endif

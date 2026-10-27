#ifndef TUI_ITINERARY_SCENE_H
#define TUI_ITINERARY_SCENE_H

#include <stdlib.h>
#include <stdbool.h>
#include <notcurses/notcurses.h>
#include "tui/scenes/scenes.h"
#include "tui/protocol.h"
#include "tui/ui/map.h"

typedef struct stop_req_payload {
        scene_t *scene;
        char *stop_name;
} stop_req_payload;

typedef struct itinerary_ctx {
        char *dep_stop_id;
        char *arr_stop_id;
        itinerary_res *itinerary_req_result;

        ui_selector *itinerary_res_selector;
        ui_selector *itinerary_detail_selector;
        ui_map *map;

        size_t focused_itinerary_idx;
} itinerary_ctx;

void free_itinerary_result(itinerary_res *r);
scene_t *create_itinerary_scene(struct notcurses *nc,
                                struct ncplane *parent_plane,
                                ui_ctx *parent_ctx);

void itinerary_receive_messages(scene_t *, ui_message *);
void itinerary_send_stop_req(ui_input *, void *);
void itinerary_send_shape_req(void *payload);
void itinerary_send_itinerary_req(void *);
void itinerary_show_assoc_selector(ui_input *, void *);
void itinerary_show_itinerary_as_selector(ui_selector *prev_selector,
                                          void *arg);

void itinerary_on_stop_select(ui_selector *, void *);
void itinerary_format_raptor_selector(struct raptor_itinerary *, ui_selector *);

#endif

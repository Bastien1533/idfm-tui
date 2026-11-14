#include <notcurses/notcurses.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "algo/raptor.h"
#include "map/shapes.h"
#include "tui/msg.h"
#include "tui/protocol.h"
#include "tui/scenes/itinerary/itinerary.h"
#include "tui/scenes/scenes.h"
#include "tui/ui/input.h"
#include "tui/ui/map.h"
#include "tui/ui/popups/route.h"
#include "tui/ui/popups/stop.h"
#include "tui/ui/selector.h"
#include "utils/hashmap.h"

static void receive_map(scene_t *scene, ui_message *message)
{
    if (scene == NULL || scene->payload == NULL || message == NULL
        || message->type != UI_SHAPE_RESULT_DATA)
        return;

    struct shape_res *res = message->as.shapes;
    if (res == NULL || res->shapes == NULL || res->n_shapes == 0)
    {
        free_flat_shapes(message->as.shapes->shapes,
                         message->as.shapes->n_shapes);
        free(message->as.shapes);
        free(message);
        switch_scene(scene->parent_ctx,
                     hash_map_get(scene->parent_ctx->scenes_map, "menu_scene"));
        return;
    }

    itinerary_ctx *m_ctx = scene->payload;

    unsigned rows, cols;
    ncplane_dim_yx(scene->plane, &rows, &cols);

    const unsigned pad = 3;
    const unsigned left = 30; // inputs/buttons width
    unsigned sel_w = 0;

    if (m_ctx->itinerary_detail_selector
        && m_ctx->itinerary_detail_selector->visible
        && m_ctx->itinerary_detail_selector->box_plane)
        sel_w = ncplane_dim_x(
            ncselector_plane(m_ctx->itinerary_detail_selector->selector));

    unsigned map_x = left + pad + (sel_w ? sel_w + pad : 0); /* 31, or 83 */
    unsigned map_y = 1;
    unsigned map_w = cols > map_x + pad ? cols - map_x - pad : 0;
    unsigned map_h = rows > map_y + pad ? rows - map_y - pad : 0;

    if (m_ctx->map != NULL)
    {
        free_flat_shapes(m_ctx->map->opts->shapes, m_ctx->map->opts->n_shapes);
        free_map(m_ctx->map);
        m_ctx->map = NULL;
    }

    if (m_ctx->map == NULL)
    {
        struct map_cell *cells = calloc(rows * cols, sizeof(struct map_cell));
        ui_map_opts motps = {
            .n_shapes = message->as.shapes->n_shapes,
            .shapes = message->as.shapes->shapes,
            .rows = map_h,
            .cols = map_w,
            .x = map_x,
            .y = map_y,
            .lat_ref = message->as.shapes->lat_ref,
        };
        struct ui_map *map = create_map(scene->plane, &motps, scene->nc);
        m_ctx->map = map;
    }

    ui_draw_map(m_ctx->map);
    free(message->as.shapes);
    free(message);
}

void itinerary_send_shape_req(void *payload)
{
    if (payload == NULL)
        return;
    scene_t *scene = payload;
    itinerary_ctx *m_c = scene->payload;
    if (m_c->itinerary_req_result == NULL)
        return;

    struct itinerary_res *iti_res = m_c->itinerary_req_result;
    if (iti_res->n_itineraries == 0 || iti_res->itineraries == NULL)
        return;
    struct raptor_itinerary *iti =
        iti_res->itineraries[m_c->focused_itinerary_idx];

    /*struct trip **trips = calloc(iti->n_trips, sizeof(struct trip *));

    size_t trip_idx = 0;
    for (size_t i = 0; i < iti->n_legs; i++) {
            struct raptor_leg *leg = iti->legs[i];
            if (leg->trip != NULL) {
                    trips[trip_idx++] = leg->trip;
            }
    } */

    scene->parent_ctx->ipc.request_seq++;
    send_itinerary_shape_req_logic(scene->parent_ctx->ipc.pipe_logic_w,
                                   scene->parent_ctx->ipc.request_seq, iti);
}

static void receive_stop_res(scene_t *scene, ui_message *message)
{
    if (scene == NULL || message == NULL
        || message->type != UI_STOP_RESULT_DATA)
        return;
    for (size_t i = 0; i < scene->ctx->n_selectors; i++)
    {
        ui_selector *selector = scene->ctx->selectors[i];
        if (selector->pending_seq == message->seq)
        {
            selector = reset_selector(selector);
            for (size_t i = 0; i < message->as.res->n_res; i++)
            {
                struct ncselector_item *n_item =
                    clone_selector_item(message->as.res->items[i]);
                int stat = uiselector_add_item(selector, n_item,
                                               message->as.res->orig_items[i]);
                if (stat == 1)
                    free_selector_item(n_item);
            }
        }
    }
    free_search_res(message->as.res);
    free(message);
}

void free_itinerary_result(itinerary_res *r)
{
    free_raptor_itineraries(r->itineraries, r->n_itineraries);
    free_selector_items(&r->search_items, &r->n_itineraries);
    free(r);
}

static void receive_itinerary_res(scene_t *scene, ui_message *message)
{
    if (scene == NULL || scene->payload == NULL || message == NULL
        || message->type != UI_ITINERARY_RESULT_DATA)
        return;
    itinerary_ctx *i_ctx = scene->payload;
    if (i_ctx->itinerary_req_result != NULL)
        free_itinerary_result(i_ctx->itinerary_req_result);
    i_ctx->itinerary_req_result = message->as.itinerary_req_res;

    if (i_ctx->itinerary_res_selector == NULL)
    {
        ui_selector_opts sel_opts = { .x = 31,
                                      .y = 0,
                                      .cols = 200,
                                      .rows = 400,
                                      .id = "itinerary_res",
                                      .onselect =
                                          itinerary_show_itinerary_as_selector,
                                      .onselect_payload = scene,
                                      .is_scrollable = true };
        ui_selector *sel =
            add_selector(scene->plane, NULL, &scene->ctx->selectors,
                         &scene->ctx->n_selectors, &sel_opts, true);
        i_ctx->itinerary_res_selector = sel;
    }
    else
    {
        reset_selector(i_ctx->itinerary_res_selector);
    }
    for (size_t i = 0; i < i_ctx->itinerary_req_result->n_itineraries; i++)
    {
        if (i_ctx->itinerary_req_result->search_items[i])
        {
            struct ncselector_item *item = clone_selector_item(
                i_ctx->itinerary_req_result->search_items[i]);
            if (item != NULL)
            {
                int stat = uiselector_add_item(i_ctx->itinerary_res_selector,
                                               item, NULL);
                if (stat == 1)
                    free_selector_item(item);
            }
        }
    }
    // free_selector_items(&message->as.itinerary_req_res->search_items,
    // &message->as.itinerary_req_res->n_itineraries);
    free(message);
}

void itinerary_receive_messages(scene_t *scene, ui_message *message)
{
    if (scene == NULL || message == NULL)
        return;
    if (message->type == UI_STOP_RESULT_DATA)
        receive_stop_res(scene, message);
    if (message->type == UI_ITINERARY_RESULT_DATA)
        receive_itinerary_res(scene, message);
    if (message->type == UI_SHAPE_RESULT_DATA)
        receive_map(scene, message);
}

void itinerary_send_stop_req(ui_input *ipt, void *payload)
{
    if (payload == NULL)
        return;
    scene_t *scene = payload;
    scene->parent_ctx->ipc.request_seq++;
    ipt->child_selector->pending_seq = scene->parent_ctx->ipc.request_seq;
    if (scene->ctx->focused_reader == NULL)
        return;
    // write a request with parenctx ipc of scene
    send_stop_req_logic(scene->parent_ctx->ipc.pipe_logic_w,
                        scene->parent_ctx->ipc.request_seq,
                        ncreader_contents(scene->ctx->focused_reader->reader));
}

void itinerary_send_itinerary_req(void *payload)
{
    if (payload == NULL)
        return;
    scene_t *scene = payload;
    itinerary_ctx *i_ctx = scene->payload;
    if (i_ctx == NULL || i_ctx->dep_stop_id == NULL
        || i_ctx->arr_stop_id == NULL)
        return;
    scene->parent_ctx->ipc.request_seq++;
    send_itinerary_req_logic(
        scene->parent_ctx->ipc.pipe_logic_w, scene->parent_ctx->ipc.request_seq,
        i_ctx->dep_stop_id, i_ctx->arr_stop_id, i_ctx->sel_dep_time);
}

#include "algo/raptor.h"
#include "data/gtfs.h"
#include "tui/protocol.h"
#include "utils/hashmap.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <notcurses/notcurses.h>
#include "tui/ui/button.h"
#include "tui/ui/input.h"
#include "tui/ui/selector.h"
#include "tui/ui/popups/stop.h"
#include "tui/ui/popups/route.h"
#include "tui/scenes/scenes.h"
#include "tui/scenes/itinerary.h"
#include "tui/msg.h"
#include <string.h>
#include "utils/utils.h"

static void go_menu(void *payload)
{
        if (payload == NULL)
                return;
        scene_t *scene = payload;
        if (scene == NULL || scene->ctx == NULL)
                return;
        switch_scene(scene->parent_ctx,
                     hash_map_get(scene->parent_ctx->scenes_map, "menu_scene"));
}

static void show_assoc_selector(ui_input *ipt, void *payload)
{
        ui_scene_ctx *scene_ctx = (ui_scene_ctx *)payload;
        if (ipt == NULL || ipt->child_selector == NULL ||
            ipt->child_selector->visible == true || scene_ctx == NULL)
                return;
        for (size_t i = 0; i < scene_ctx->n_selectors; i++) {
                if (scene_ctx->selectors[i]->parent_input != NULL || 1 == 1)
                        hide_selector(scene_ctx->selectors[i]);
        }
        //ipt->child_selector = show_selector(ipt->child_selector);
}

void add_itinerary_item_selector(ui_selector *selector, char type, char *id,
                                 const char *fmt, ...)
{
        va_list arglist;
        va_start(arglist, fmt);
        char *desc;
        char *opt;

        if (vasprintf(&desc, fmt, arglist) != -1) {
                struct ncselector_item *item =
                        calloc(1, sizeof(struct ncselector_item));
                if (item == NULL) {
                        va_end(arglist);
                        return;
                }
                item->desc = desc;
                int stat = (id == NULL) ? asprintf(&opt, "%c", type) :
                                          asprintf(&opt, "%c|%s", type, id);
                if (stat == -1) {
                        item->option = NULL;
                        free_selector_item(item);
                } else {
                        item->option = opt;
                        stat = uiselector_add_item(selector, item);
                        if (stat == 1)
                                free_selector_item(item);
                }
        }
        va_end(arglist);
}

static void format_raptor_selector(struct raptor_itinerary *itinerary,
                                   ui_selector *sel)
{
        if (itinerary == NULL) {
                return;
        }
        for (size_t j = itinerary->n_legs; j > 0; j--) {
                struct raptor_leg *leg = itinerary->legs[j - 1];
                if (leg == NULL || leg->from == NULL || leg->to == NULL ||
                    leg->from->stop == NULL || leg->to->stop == NULL) {
                        continue;
                }
                if (leg->trip != NULL) {
                        char *dep_stop_hhmmss = print_hhmmss(leg->dep_time);
                        char *arr_stop_hhmmss = print_hhmmss(leg->arr_time);
                        add_itinerary_item_selector(sel, 'S',
                                                    leg->from->stop->stop_id,
                                                    "%s || %s (%s)",
                                                    dep_stop_hhmmss,
                                                    leg->from->stop->stop_name,
                                                    leg->from->stop->stop_id);
                        add_itinerary_item_selector(
                                sel, 'R', leg->trip->route->route_id,
                                "         || %s - %s ",
                                leg->trip->route->route_short_name,
                                leg->trip->trip_headsign);
                        add_itinerary_item_selector(sel, '\0', NULL,
                                                    "         ||");
                        add_itinerary_item_selector(sel, 'S',
                                                    leg->to->stop->stop_id,
                                                    "%s || %s (%s)",
                                                    arr_stop_hhmmss,
                                                    leg->to->stop->stop_name,
                                                    leg->to->stop->stop_id);
                        free(dep_stop_hhmmss);
                        free(arr_stop_hhmmss);
                }
                if (leg->transfer != NULL && j != itinerary->n_legs && j != 1) {
                        add_itinerary_item_selector(
                                sel, '\0', NULL, "Transfer | %hu min \n",
                                (leg->transfer->min_transfer_time / 60));
                }
        }
}

static void escape_focused_itinerary(ui_selector *src, void *arg)
{
        if (src == NULL || arg == NULL)
                return;
        scene_t *scene = arg;
        itinerary_ctx *i_ctx = scene->payload;
        hide_selector(src);
        reset_selector(i_ctx->itinerary_res_selector);
        for (size_t i = 0; i < i_ctx->itinerary_req_result->n_itineraries;
             i++) {
                if (i_ctx->itinerary_req_result->search_items[i]) {
                        struct ncselector_item *item = clone_selector_item(
                                i_ctx->itinerary_req_result->search_items[i]);
                        if (item != NULL) {
                                int stat = uiselector_add_item(
                                        i_ctx->itinerary_res_selector, item);
                                if (stat == 1)
                                        free_selector_item(item);
                        }
                }
        }
}

static void show_stop_popup(scene_t *scene, char *stop_id,
                            struct raptor_itinerary *itinerary)
{
        if (scene == NULL || stop_id == NULL || itinerary == NULL)
                return;
        for (size_t i = 0; i < itinerary->n_legs; i++) {
                struct raptor_leg *leg = itinerary->legs[i];
                if (leg == NULL)
                        return;
                if (strcmp(leg->from->stop->stop_id, stop_id) == 0) {
                        create_stop_popup(scene, leg->from->stop);
                        return;
                }
                if (strcmp(leg->to->stop->stop_id, stop_id) == 0) {
                        create_stop_popup(scene, leg->to->stop);
                        return;
                }
        }
}

static void show_route_popup(scene_t *scene, char *route_id,
                             struct raptor_itinerary *itinerary)
{
        if (scene == NULL || route_id == NULL || itinerary == NULL)
                return;
        for (size_t i = 0; i < itinerary->n_legs; i++) {
                struct raptor_leg *leg = itinerary->legs[i];
                if (leg == NULL || leg->trip == NULL ||
                    leg->trip->route == NULL)
                        continue;
                if (strcmp(leg->trip->route->route_id, route_id) == 0) {
                        create_route_popup(scene, leg->trip->route);
                        return;
                }
        }
}

static void on_select_itinerary_detail_selector(ui_selector *src, void *arg)
{
        if (src == NULL || arg == NULL)
                return;
        scene_t *scene = arg;
        itinerary_ctx *i_ctx = scene->payload;
        const char *selected_str = ncselector_selected(src->selector);
        size_t len = strlen(selected_str);
        if (len > 0) {
                char *search_str = strdup(selected_str);
                const char *delim = "|";
                char *saveptr;
                char *token = strtok_r(search_str, delim, &saveptr);

                int i = 0;
                char select_type = 0;

                while (token != NULL) {
                        if (i == 0)
                                select_type = token[0];
                        if (i == 1) {
                                if (select_type == 'S')
                                        show_stop_popup(
                                                scene, token,
                                                i_ctx->itinerary_req_result->itineraries
                                                        [i_ctx->focused_itinerary_idx]);
                                if (select_type == 'R')
                                        show_route_popup(
                                                scene, token,
                                                i_ctx->itinerary_req_result->itineraries
                                                        [i_ctx->focused_itinerary_idx]);
                        }

                        token = strtok_r(NULL, delim, &saveptr);
                        i++;
                }
        }
}

static void show_itinerary_as_selector(ui_selector *prev_selector, void *arg)
{
        if (arg == NULL)
                return;
        scene_t *scene = arg;
        if (arg == NULL || scene == NULL || scene->payload == NULL ||
            prev_selector == NULL)
                return;
        int idx = atoi(ncselector_selected(prev_selector->selector));
        itinerary_ctx *i_ctx = scene->payload;
        if (i_ctx->itinerary_req_result == NULL ||
            i_ctx->itinerary_req_result->itineraries == NULL)
                return;

        hide_selector(prev_selector);
        if (i_ctx->itinerary_detail_selector == NULL) {
                ui_selector_opts sel_opts = {
                        .x = 31,
                        .y = 0,
                        .cols = 200,
                        .rows = 400,
                        .id = "itinerary",
                        .is_scrollable = true,
                        .onselect = on_select_itinerary_detail_selector,
                        .onselect_payload = scene,
                        .onescape = escape_focused_itinerary,
                        .onescape_payload = scene
                };
                ui_selector *sel =
                        add_selector(scene->plane, NULL, &scene->ctx->selectors,
                                     &scene->ctx->n_selectors, &sel_opts, true);
                i_ctx->itinerary_detail_selector = sel;
        } else {
                reset_selector(i_ctx->itinerary_detail_selector);
        }
        if (i_ctx->itinerary_detail_selector == NULL)
                return;
        format_raptor_selector(i_ctx->itinerary_req_result->itineraries[idx],
                               i_ctx->itinerary_detail_selector);
        i_ctx->focused_itinerary_idx = idx;
}

static void receive_stop_res(scene_t *scene, ui_message *message)
{
        if (scene == NULL || message == NULL ||
            message->type != UI_STOP_RESULT_DATA)
                return;
        ncplane_printf(scene->plane, "%s", "received");
        for (size_t i = 0; i < scene->ctx->n_selectors; i++) {
                ui_selector *selector = scene->ctx->selectors[i];
                if (selector->pending_seq == message->seq) {
                        selector = reset_selector(selector);
                        for (size_t i = 0; i < message->as.res->n_res; i++) {
                                struct ncselector_item *n_item =
                                        clone_selector_item(
                                                message->as.res->items[i]);
                                int stat =
                                        uiselector_add_item(selector, n_item);
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
        if (scene == NULL || scene->payload == NULL || message == NULL ||
            message->type != UI_ITINERARY_RESULT_DATA)
                return;
        ncplane_printf(scene->plane, "%s", "received itinerary_data\n");
        itinerary_ctx *i_ctx = scene->payload;
        if (i_ctx->itinerary_req_result != NULL)
                free_itinerary_result(i_ctx->itinerary_req_result);
        i_ctx->itinerary_req_result = message->as.itinerary_req_res;

        if (i_ctx->itinerary_res_selector == NULL) {
                ui_selector_opts sel_opts = {
                        .x = 31,
                        .y = 0,
                        .cols = 200,
                        .rows = 400,
                        .id = "itinerary_res",
                        .onselect = show_itinerary_as_selector,
                        .onselect_payload = scene,
                        .is_scrollable = true
                };
                ui_selector *sel =
                        add_selector(scene->plane, NULL, &scene->ctx->selectors,
                                     &scene->ctx->n_selectors, &sel_opts, true);
                i_ctx->itinerary_res_selector = sel;
        } else {
                reset_selector(i_ctx->itinerary_res_selector);
        }
        for (size_t i = 0; i < i_ctx->itinerary_req_result->n_itineraries;
             i++) {
                if (i_ctx->itinerary_req_result->search_items[i]) {
                        struct ncselector_item *item = clone_selector_item(
                                i_ctx->itinerary_req_result->search_items[i]);
                        if (item != NULL) {
                                int stat = uiselector_add_item(
                                        i_ctx->itinerary_res_selector, item);
                                if (stat == 1)
                                        free_selector_item(item);
                        }
                }
        }
        //free_selector_items(&message->as.itinerary_req_res->search_items, &message->as.itinerary_req_res->n_itineraries);
        free(message);
}

static void receive_messages(scene_t *scene, ui_message *message)
{
        if (scene == NULL || message == NULL)
                return;
        if (message->type == UI_STOP_RESULT_DATA)
                receive_stop_res(scene, message);
        if (message->type == UI_ITINERARY_RESULT_DATA)
                receive_itinerary_res(scene, message);
}

static void send_stop_req(ui_input *ipt, void *payload)
{
        if (payload == NULL)
                return;
        scene_t *scene = payload;
        scene->parent_ctx->ipc.request_seq++;
        ipt->child_selector->pending_seq = scene->parent_ctx->ipc.request_seq;
        ncplane_printf(scene->plane, "%s", "sent");
        if (scene->ctx->focused_reader == NULL)
                return;
        // write a request with parenctx ipc of scene
        send_stop_req_logic(
                scene->parent_ctx->ipc.pipe_logic_w,
                scene->parent_ctx->ipc.request_seq,
                ncreader_contents(scene->ctx->focused_reader->reader));
}

static void send_itinerary_req(void *payload)
{
        if (payload == NULL)
                return;
        scene_t *scene = payload;
        itinerary_ctx *i_ctx = scene->payload;
        if (i_ctx == NULL || i_ctx->dep_stop_id == NULL ||
            i_ctx->arr_stop_id == NULL)
                return;
        scene->parent_ctx->ipc.request_seq++;
        send_itinerary_req_logic(scene->parent_ctx->ipc.pipe_logic_w,
                                 scene->parent_ctx->ipc.request_seq,
                                 i_ctx->dep_stop_id, i_ctx->arr_stop_id);
}

static void on_stop_select(ui_selector *sel, void *arg)
{
        if (sel == NULL || sel->selector == NULL)
                return;
        const char *selected = ncselector_selected(sel->selector);
        if (arg != NULL && selected != NULL) {
                char **slot = arg;
                free(*slot);
                *slot = strdup(selected);
                ncplane_printf(sel->parent_plane, "%s", *slot);
        }
        hide_selector(sel);
}

void scene_itinerary_on_enter(scene_t *scene)
{
        if (scene == NULL || scene->ctx == NULL || scene->parent_ctx == NULL ||
            scene->parent == NULL) {
                return;
        }

        unsigned rows, cols;
        ncplane_dim_yx(scene->parent, &rows, &cols);
        struct ncplane_options nopts = { .y = 0,
                                         .x = 0,
                                         .rows = rows,
                                         .cols = cols,
                                         .name = "scene_itinerary_plane" };
        on_scene_enter(scene, &nopts);

        ui_button_opts btn_opts = {
                .h = 3,
                .w = 30,
                .x = 0,
                .y = 15,
                .id = "test",
                .onclick = send_itinerary_req,
                .onclick_payload = scene,
        };

        add_button(scene->plane, &scene->ctx->buttons, &scene->ctx->n_buttons,
                   scene->ctx->button_map, &btn_opts, "Get Itinerary");

        btn_opts.y = 20;
        btn_opts.onclick = go_menu;

        add_button(scene->plane, &scene->ctx->buttons, &scene->ctx->n_buttons,
                   scene->ctx->button_map, &btn_opts, "Back to Menu");

        for (size_t i = 0; i < 10; i += 5) {
                ui_input_opts ipt_opts = {
                        .x = 0,
                        .y = i,
                        .cols = 30,
                        .rows = 3,
                        .id = "test",
                        .onsubmit = send_stop_req,
                        .onsubmit_payload = scene,
                        .ontype = show_assoc_selector,
                        .ontype_payload = scene->ctx,
                };
                ui_input *ipt = add_input(scene->plane, &scene->ctx->inputs,
                                          &scene->ctx->n_inputs, &ipt_opts);
                if (ipt == NULL)
                        continue;
                itinerary_ctx *itinerary_ctx = scene->payload;
                ui_selector_opts sel_opts = { .x = 31,
                                              .y = 0,
                                              .cols = 200,
                                              .rows = 400,
                                              .id = "test",
                                              .onselect = on_stop_select,
                                              .is_scrollable = true };
                sel_opts.onselect_payload =
                        (i == 0) ? (void *)&itinerary_ctx->dep_stop_id :
                                   (void *)&itinerary_ctx->arr_stop_id;

                ui_selector *sel = add_selector(scene->plane, ipt,
                                                &scene->ctx->selectors,
                                                &scene->ctx->n_selectors,
                                                &sel_opts, false);
                ipt->child_selector = sel;
        }
        return;
}

void on_scene_itinerary_exit(scene_t *scene)
{
        if (scene->payload != NULL) {
                itinerary_ctx *i_ctx = scene->payload;
                if (i_ctx->dep_stop_id)
                        free(i_ctx->dep_stop_id);
                if (i_ctx->arr_stop_id)
                        free(i_ctx->arr_stop_id);
                if (i_ctx->itinerary_req_result) {
                        free_itinerary_result(i_ctx->itinerary_req_result);
                }
                i_ctx->dep_stop_id = NULL;
                i_ctx->arr_stop_id = NULL;
                i_ctx->itinerary_req_result = NULL;
                i_ctx = NULL;
        }
        on_scene_exit(scene);
        if (scene->payload != NULL) {
                itinerary_ctx *i_ctx = scene->payload;
                i_ctx->itinerary_res_selector = NULL;
                i_ctx->itinerary_detail_selector = NULL;
        }
}

scene_t *create_itinerary_scene(struct notcurses *nc,
                                struct ncplane *parent_plane,
                                ui_ctx *parent_ctx)
{
        if (nc == NULL || parent_plane == NULL)
                return NULL;

        ui_scene_ctx *ctx = init_scene_ctx(NULL);
        if (ctx == NULL)
                return NULL;

        scene_t *scene = calloc(1, sizeof(scene_t));
        if (scene == NULL) {
                free_hashmap(ctx->button_map);
                free(ctx);
                return NULL;
        }
        scene->ctx = ctx;

        itinerary_ctx *itinerary_payload = calloc(1, sizeof(itinerary_ctx));
        if (itinerary_payload == NULL) {
                destroy_scene(scene);
                return NULL;
        }

        scene->type = UI_ITINERARY_SCENE;
        scene->ctx = ctx;
        scene->parent_ctx = parent_ctx;
        scene->parent = parent_plane;
        scene->payload = itinerary_payload;
        scene->handle_input = handle_scene_input;
        scene->on_enter = scene_itinerary_on_enter;
        scene->on_exit = on_scene_itinerary_exit;
        scene->handle_message = receive_messages;
        scene->id = strdup("itinerary_scene");
        scene->nc = nc;

        return scene;
}

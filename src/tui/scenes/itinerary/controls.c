#include "tui/ui/map.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <notcurses/notcurses.h>

#include "algo/raptor.h"
#include "data/gtfs.h"
#include "tui/protocol.h"
#include "tui/ui/input.h"
#include "tui/ui/selector.h"
#include "tui/ui/popups/stop.h"
#include "tui/ui/popups/route.h"
#include "tui/scenes/scenes.h"
#include "tui/scenes/itinerary/itinerary.h"

void itinerary_show_assoc_selector(ui_input *ipt, void *payload)
{
        ui_scene_ctx *scene_ctx = (ui_scene_ctx *)payload;
        if (ipt == NULL || ipt->child_selector == NULL || scene_ctx == NULL)
                return;
        for (size_t i = 0; i < scene_ctx->n_selectors; i++) {
                if (scene_ctx->selectors[i]->parent_input != NULL || 1 == 1)
                        hide_selector(scene_ctx->selectors[i]);
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
                                        i_ctx->itinerary_res_selector, item,
                                        NULL);
                                if (stat == 1)
                                        free_selector_item(item);
                        }
                }
        }
        if (i_ctx->map != NULL) {
                free_flat_shapes(i_ctx->map->opts->shapes,
                                 i_ctx->map->opts->n_shapes);
                free_map(i_ctx->map);
                i_ctx->map = NULL;
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

void itinerary_show_itinerary_as_selector(ui_selector *prev_selector, void *arg)
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
                        .cols = 50,
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
        itinerary_format_raptor_selector(
                i_ctx->itinerary_req_result->itineraries[idx],
                i_ctx->itinerary_detail_selector);
        i_ctx->focused_itinerary_idx = idx;
        itinerary_send_shape_req(scene);
}

void itinerary_on_stop_select(ui_selector *sel, void *arg)
{
        if (sel == NULL || sel->selector == NULL)
                return;
        const char *selected = ncselector_selected(sel->selector);
        if (arg != NULL && selected != NULL) {
                char **slot = arg;
                free(*slot);
                *slot = strdup(selected);
                for (size_t i = 0; i < sel->n_items; i++) {
                        if (strcmp(sel->items[i]->option, selected) == 0) {
                                struct stop *s = sel->orig_items[i];
                                set_input_text(sel->parent_input, s->stop_name);
                        }
                }
        }
        hide_selector(sel);
}

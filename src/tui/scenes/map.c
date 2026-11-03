#include "tui/ui/map.h"
#include "data/rules.h"
#include "map/map.h"
#include "map/shapes.h"
#include "tui/msg.h"
#include "tui/protocol.h"
#include "tui/ui/button_selector.h"
#include "utils/hashmap.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <notcurses/notcurses.h>
#include "tui/scenes/scenes.h"
#include <string.h>
#include "tui/scenes/map.h"

static void receive_shape_res(scene_t *scene, ui_message *message)
{
        if (scene == NULL || scene->payload == NULL || message == NULL ||
            message->type != UI_SHAPE_RESULT_DATA)
                return;

        struct shape_res *res = message->as.shapes;
        map_ctx *m_ctx = scene->payload;

        if (res == NULL || res->shapes == NULL || res->n_shapes == 0 ||
            m_ctx->pending_seq != message->seq) {
                if (res != NULL) {
                        free_flat_shapes(message->as.shapes->shapes,
                                         message->as.shapes->n_shapes);
                        free(message->as.shapes);
                }
                free(message);
                switch_scene(scene->parent_ctx,
                             hash_map_get(scene->parent_ctx->scenes_map,
                                          "menu_scene"));
                return;
        }

        unsigned rows, cols;
        ncplane_dim_yx(scene->parent, &rows, &cols);

        if (m_ctx->map) {
                free_flat_shapes(m_ctx->map->opts->shapes,
                                 m_ctx->map->opts->n_shapes);
                free_map(m_ctx->map);
                m_ctx->map = NULL;
        }
        if (m_ctx->map == NULL) {
                ui_map_opts motps = {
                        .n_shapes = message->as.shapes->n_shapes,
                        .shapes = message->as.shapes->shapes,
                        .rows = rows - 2,
                        .cols = cols - 5,
                        .x = 2,
                        .y = 1,
                        .lat_ref = message->as.shapes->lat_ref,
                };
                struct ui_map *map =
                        create_map(scene->plane, &motps, scene->nc);
                m_ctx->map = map;
                if (m_ctx->sel)
                        ncplane_move_top(m_ctx->sel->plane);
                free(message->as.shapes);
        } else {
                free_flat_shapes(message->as.shapes->shapes,
                                 message->as.shapes->n_shapes);
                free(message->as.shapes);
        }

        ui_draw_map(m_ctx->map);
        free(message);
}

static void send_shape_req(void *payload)
{
        if (payload == NULL)
                return;
        scene_t *scene = payload;
        map_ctx *m_c = scene->payload;
        m_c->pending_seq = scene->parent_ctx->ipc.request_seq;

        send_shape_req_logic(scene->parent_ctx->ipc.pipe_logic_w,
                             scene->parent_ctx->ipc.request_seq,
                             m_c->included_modes);
}

static void receive_messages(scene_t *scene, ui_message *message)
{
        if (scene == NULL || message == NULL)
                return;
        if (message->type == UI_SHAPE_RESULT_DATA)
                receive_shape_res(scene, message);
}

void mode_selector_on_select(char *id, void *payload)
{
        if (payload == NULL)
                return;
        scene_t *scene = payload;
        if (scene == NULL || scene->ctx == NULL || scene->payload == NULL)
                return;
        map_ctx *m_ctx = scene->payload;
        if (strcmp(id, "metro") == 0)
                m_ctx->included_modes ^= METRO;
        if (strcmp(id, "bus") == 0)
                m_ctx->included_modes ^= BUS;
        if (strcmp(id, "tram") == 0)
                m_ctx->included_modes ^= TRAMWAY;
        if (strcmp(id, "comut_rail") == 0)
                m_ctx->included_modes ^= COMMUTER_RAIL;
        if (strcmp(id, "rail") == 0)
                m_ctx->included_modes ^= RAIL | REGIONAL_RAIL | HS_RAIL;
        if (strcmp(id, "other") == 0)
                m_ctx->included_modes ^= CABLE | UNKNOWN | AIRPORT_RAIL | TAXI |
                                         FUNICULAR;
        send_shape_req(scene);
}

void scene_map_on_enter(scene_t *scene)
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
                                         .name = "scene_map_plane" };
        on_scene_enter(scene, &nopts);

        struct map_ctx *m_ctx = scene->payload;
        m_ctx->included_modes = METRO;
        if (!m_ctx->map)
                send_shape_req(scene);

        ui_btn_selector_item *items = calloc(6, sizeof(ui_btn_selector_item));
        uint64_t bitmask = 0;
        ui_btn_selector_item item1 = {
                .desc = "Bus",
                .id = "bus",
        };
        ui_btn_selector_item item2 = {
                .desc = "Métro",
                .id = "metro",
        };
        bitmask |= (1 << 1);
        ui_btn_selector_item item3 = {
                .desc = "Tramway",
                .id = "tram",
        };
        ui_btn_selector_item item4 = {
                .desc = "RER",
                .id = "comut_rail",
        };
        ui_btn_selector_item item5 = {
                .desc = "Rail Lourd",
                .id = "rail",
        };
        ui_btn_selector_item item6 = {
                .desc = "Autres",
                .id = "other",
        };
        items[0] = item1;
        items[1] = item2;
        items[2] = item3;
        items[3] = item4;
        items[4] = item5;
        items[5] = item6;

        ui_btn_selector_opts sopts = { .items = items,
                                       .n_items = 6,
                                       .pad = 0,
                                       .selection_bitmask = bitmask,
                                       .is_radio = false,
                                       .hovered = -1,
                                       .id = "feur",
                                       .onselect = mode_selector_on_select,
                                       .onselect_payload = scene };
        ui_btn_selector *sel = create_btn_selector(scene->plane, &sopts);
        m_ctx->sel = sel;

        return;
}

void handle_map_scene_input(scene_t *self, const ncinput *ni, uint32_t key)
{
        if (self == NULL || ni == NULL || self->ctx == NULL)
                return;

        struct map_ctx *m_ctx = self->payload;


        bool is_repeat = (ni->evtype == NCTYPE_REPEAT);
        bool is_release = (ni->evtype == NCTYPE_RELEASE);
        bool is_in_sel = (m_ctx != NULL && m_ctx->sel != NULL) &&
                         ((ni->x) >= m_ctx->sel->opts->x &&
                          (size_t)(ni->x) <= m_ctx->sel->opts->x +
                                                     m_ctx->sel->opts->cols) &&
                         ((ni->y) >= m_ctx->sel->opts->y &&
                          (size_t)(ni->y) <=
                                  m_ctx->sel->opts->y + m_ctx->sel->opts->rows);
        bool is_in_map = (m_ctx != NULL && m_ctx->map != NULL) &&
                         ((ni->x) >= m_ctx->map->opts->x &&
                          (size_t)(ni->x) <= m_ctx->map->opts->x +
                                                     m_ctx->map->opts->cols) &&
                         ((ni->y) >= m_ctx->map->opts->y &&
                          (size_t)(ni->y) <=
                                  m_ctx->map->opts->y + m_ctx->map->opts->rows);
        if (is_release)
                return;
        if (key == NCKEY_ESC && !is_repeat) {
                switch_scene(self->parent_ctx,
                             hash_map_get(self->parent_ctx->scenes_map,
                                          "menu_scene"));
                return;
        }

        if (key == NCKEY_BUTTON1 && is_in_sel)
        {
            m_ctx->selector_focused = true;
            set_selector_focused(m_ctx->sel);
        }
        else if (key == NCKEY_BUTTON1 && is_in_map)
        {
            m_ctx->selector_focused = false;
            set_selector_unfocused(m_ctx->sel);
        }
        
        if (m_ctx != NULL && m_ctx->sel != NULL && m_ctx->selector_focused) {
            handle_btn_selector_input(m_ctx->sel, ni, key);
        }
        if (m_ctx != NULL && m_ctx->map != NULL && !m_ctx->selector_focused) {
                handle_map_input(ni, key, &m_ctx->map->view);
                if (key != NCKEY_MOTION)
                        ui_draw_map(m_ctx->map);
                if (m_ctx->sel)
                        ncplane_move_top(m_ctx->sel->plane);
        }
        handle_scene_input(self, ni, key);
}

void on_scene_map_exit(scene_t *scene)
{
        map_ctx *m_ctx = scene->payload;
        if (m_ctx != NULL && m_ctx->map != NULL) {
                free_flat_shapes(m_ctx->map->opts->shapes,
                                 m_ctx->map->opts->n_shapes);
                m_ctx->map->opts->shapes = NULL;
                m_ctx->map->opts->n_shapes = 0;
                free_map(m_ctx->map); /* frees plane + opts + cells + struct */
                m_ctx->map = NULL;
                m_ctx->pending_seq = 0;
        }
        if (m_ctx && m_ctx->sel) {
                free_btn_selector(m_ctx->sel);
                m_ctx->sel = NULL;
        }
        on_scene_exit(scene);
}

scene_t *create_map_scene(struct notcurses *nc, struct ncplane *parent_plane,
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

        map_ctx *map_payload = calloc(1, sizeof(map_ctx));
        if (map_payload == NULL) {
                destroy_scene(scene);
                return NULL;
        }

        scene->type = UI_MAP_SCENE;
        scene->ctx = ctx;
        scene->parent_ctx = parent_ctx;
        scene->parent = parent_plane;
        scene->handle_input = handle_map_scene_input;
        scene->handle_message = receive_messages;
        scene->on_enter = scene_map_on_enter;
        scene->payload = map_payload;
        scene->on_exit = on_scene_map_exit;
        scene->id = strdup("map_scene");
        scene->nc = nc;

        return scene;
}

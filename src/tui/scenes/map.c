#include "tui/ui/map.h"
#include "map/map.h"
#include "map/shapes.h"
#include "tui/msg.h"
#include "tui/protocol.h"
#include "utils/hashmap.h"
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
        if (res == NULL || res->shapes == NULL || res->n_shapes == 0) {
                free(message);
                switch_scene(scene->parent_ctx,
                             hash_map_get(scene->parent_ctx->scenes_map,
                                          "menu_scene"));
                return;
        }

        map_ctx *m_ctx = scene->payload;
        if (m_ctx->pending_seq != message->seq)
                return;

        unsigned rows, cols;
        ncplane_dim_yx(scene->parent, &rows, &cols);

        if (m_ctx->map == NULL) {
                struct map_cell *cells =
                        calloc(rows * cols, sizeof(struct map_cell));
                ui_map_opts motps = {
                        .n_shapes = message->as.shapes->n_shapes,
                        .shapes = message->as.shapes->shapes,
                        .rows = rows - 5,
                        .cols = cols - 10,
                        .x = 5,
                        .y = 2,
                        .lat_ref = message->as.shapes->lat_ref,
                };
                struct ui_map *map =
                        create_map(scene->plane, &motps, scene->nc);
                m_ctx->map = map;
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
                             scene->parent_ctx->ipc.request_seq);
}

static void receive_messages(scene_t *scene, ui_message *message)
{
        if (scene == NULL || message == NULL)
                return;
        if (message->type == UI_SHAPE_RESULT_DATA)
                receive_shape_res(scene, message);
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

        send_shape_req(scene);
        return;
}

void handle_map_scene_input(scene_t *self, const ncinput *ni, uint32_t key)
{
        if (self == NULL || ni == NULL || self->ctx == NULL)
                return;
        bool is_repeat = (ni->evtype == NCTYPE_REPEAT);
        if (key == NCKEY_ESC && !is_repeat) {
                switch_scene(self->parent_ctx,
                             hash_map_get(self->parent_ctx->scenes_map,
                                          "menu_scene"));
                return;
        }

        struct map_ctx *m_ctx = self->payload;
        if (m_ctx != NULL && m_ctx->map != NULL) {
                handle_map_input(ni, key, &m_ctx->map->view);
                ui_draw_map(m_ctx->map);
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

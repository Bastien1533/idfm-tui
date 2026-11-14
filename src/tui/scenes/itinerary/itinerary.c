#include "tui/scenes/itinerary/itinerary.h"

#include <notcurses/notcurses.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "map/map.h"
#include "tui/scenes/scenes.h"
#include "tui/ui/button.h"
#include "tui/ui/input.h"
#include "tui/ui/map.h"
#include "tui/ui/popups/route.h"
#include "tui/ui/popups/stop.h"
#include "tui/ui/selector.h"
#include "utils/hashmap.h"
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

void handle_itinerary_scene_input(scene_t *self, const ncinput *ni,
                                  uint32_t key)
{
    if (self == NULL || ni == NULL || self->ctx == NULL)
        return;
    bool is_repeat = (ni->evtype == NCTYPE_REPEAT);
    bool is_press = (ni->evtype == NCTYPE_PRESS);

    struct itinerary_ctx *m_ctx = self->payload;

    bool is_map_scroll_keys =
        key == NCKEY_SCROLL_DOWN || key == NCKEY_SCROLL_UP;
    bool is_map_click_keys = key == NCKEY_BUTTON1;
    bool is_map_ctrl_keys = key == NCKEY_LEFT || key == NCKEY_RIGHT
        || key == NCKEY_UP || key == NCKEY_DOWN;
    bool is_in_map = (m_ctx != NULL && m_ctx->map != NULL)
        && ((ni->x) >= m_ctx->map->opts->x
            && (size_t)(ni->x) <= m_ctx->map->opts->x + m_ctx->map->opts->cols)
        && ((ni->y) >= m_ctx->map->opts->y
            && (size_t)(ni->y) <= m_ctx->map->opts->y + m_ctx->map->opts->rows);
    bool has_map = m_ctx && m_ctx->map;

    if (has_map && is_map_click_keys && is_press)
        m_ctx->map->focused = is_in_map;
    if (has_map
        && ((is_map_scroll_keys && is_in_map)
            || (m_ctx->map->focused && is_map_ctrl_keys)))
    {
        handle_map_input(ni, key, &m_ctx->map->view);
        ui_draw_map(m_ctx->map);
    }
    else
    {
        handle_scene_input(self, ni, key);
    }
}

void scene_itinerary_on_enter(scene_t *scene)
{
    if (scene == NULL || scene->ctx == NULL || scene->parent_ctx == NULL
        || scene->parent == NULL)
    {
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
        .onclick = itinerary_send_itinerary_req,
        .onclick_payload = scene,
    };

    add_button(scene->plane, &scene->ctx->buttons, &scene->ctx->n_buttons,
               scene->ctx->button_map, &btn_opts, "Get Itinerary");

    btn_opts.y = 20;
    btn_opts.onclick = go_menu;

    add_button(scene->plane, &scene->ctx->buttons, &scene->ctx->n_buttons,
               scene->ctx->button_map, &btn_opts, "Back to Menu");

    itinerary_ctx *itinerary_ctx = scene->payload;
    for (size_t i = 0; i < 10; i += 5)
    {
        ui_input_opts ipt_opts = {
            .x = 0,
            .y = i,
            .cols = 30,
            .rows = 3,
            .id = "test",
            .onsubmit = itinerary_send_stop_req,
            .onsubmit_payload = scene,
            .ontype = itinerary_show_assoc_selector,
            .ontype_payload = scene->ctx,
        };
        ui_input *ipt = add_input(scene->plane, &scene->ctx->inputs,
                                  &scene->ctx->n_inputs, &ipt_opts);
        if (ipt == NULL)
            continue;

        ui_selector_opts sel_opts = { .x = 31,
                                      .y = 0,
                                      .cols = 200,
                                      .rows = 400,
                                      .id = "test",
                                      .onselect = itinerary_on_stop_select,
                                      .is_scrollable = true };
        sel_opts.onselect_payload = (i == 0)
            ? (void *)&itinerary_ctx->dep_stop_id
            : (void *)&itinerary_ctx->arr_stop_id;

        ui_selector *sel =
            add_selector(scene->plane, ipt, &scene->ctx->selectors,
                         &scene->ctx->n_selectors, &sel_opts, false);
        ipt->child_selector = sel;
    }

    ui_input_opts ipt_opts = {
        .x = 0,
        .y = 10,
        .cols = 30,
        .rows = 3,
        .id = "time",
        .onsubmit = itinerary_set_dep_time,
        .onsubmit_payload = scene,
        .ontype = NULL,
        .ontype_payload = scene->ctx,
    };
    ui_input *ipt = add_input(scene->plane, &scene->ctx->inputs,
                              &scene->ctx->n_inputs, &ipt_opts);
    if (ipt)
    {
        char *time_str = print_hhmm(itinerary_ctx->sel_dep_time);
        set_input_text(ipt, time_str);
        free(time_str);
    }
    return;
}

void on_scene_itinerary_exit(scene_t *scene)
{
    if (scene->payload != NULL)
    {
        itinerary_ctx *i_ctx = scene->payload;
        if (i_ctx->dep_stop_id)
            free(i_ctx->dep_stop_id);
        if (i_ctx->arr_stop_id)
            free(i_ctx->arr_stop_id);
        if (i_ctx->itinerary_req_result)
        {
            free_itinerary_result(i_ctx->itinerary_req_result);
        }
        if (i_ctx->map)
        {
            free_flat_shapes(i_ctx->map->opts->shapes,
                             i_ctx->map->opts->n_shapes);
            free_map(i_ctx->map);
            i_ctx->map = NULL;
        }
        i_ctx->dep_stop_id = NULL;
        i_ctx->arr_stop_id = NULL;
        i_ctx->itinerary_req_result = NULL;
        i_ctx = NULL;
    }
    on_scene_exit(scene);
    if (scene->payload != NULL)
    {
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
    if (scene == NULL)
    {
        free_hashmap(ctx->button_map);
        free(ctx);
        return NULL;
    }
    scene->ctx = ctx;

    itinerary_ctx *itinerary_payload = calloc(1, sizeof(itinerary_ctx));
    if (itinerary_payload == NULL)
    {
        destroy_scene(scene);
        return NULL;
    }
    itinerary_payload->sel_dep_time = 8 * 3600;

    scene->type = UI_ITINERARY_SCENE;
    scene->ctx = ctx;
    scene->parent_ctx = parent_ctx;
    scene->parent = parent_plane;
    scene->payload = itinerary_payload;
    scene->handle_input = handle_itinerary_scene_input;
    scene->on_enter = scene_itinerary_on_enter;
    scene->on_exit = on_scene_itinerary_exit;
    scene->handle_message = itinerary_receive_messages;
    scene->id = strdup("itinerary_scene");
    scene->nc = nc;

    return scene;
}

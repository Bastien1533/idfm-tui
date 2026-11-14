#include "tui/scenes/scenes.h"

#include <notcurses/notcurses.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "tui/scenes/utils.h"
#include "tui/ui/button.h"
#include "tui/ui/input.h"
#include "tui/ui/popups/popups.h"
#include "tui/ui/selector.h"
#include "tui/utils.h"
#include "utils/hashmap.h"

ui_ctx *init_ctx(int pipe_ui_r, int pipe_logic_w)
{
    ui_ctx *ctx = malloc(1 * sizeof(ui_ctx));
    if (!ctx)
        return NULL;

    scene_t **scenes = NULL;
    struct hash_map *scene_map = init_hashmap(20);
    size_t n_scenes = 0;

    ctx->scenes_map = scene_map;
    ctx->scenes = scenes;
    ctx->n_scenes = n_scenes;

    ctx->should_quit = false;
    ctx->next_scene = NULL;

    ctx->ipc.pipe_logic_w = pipe_logic_w;
    ctx->ipc.pipe_ui_r = pipe_ui_r;
    ctx->ipc.request_seq = 1;

    return ctx;
}

bool add_scene(scene_t *scene, ui_ctx *ctx)
{
    if (ctx == NULL || scene == NULL)
        return false;

    scene_t **new_scenes =
        realloc(ctx->scenes, (ctx->n_scenes + 1) * sizeof(scene_t *));
    if (new_scenes == NULL)
    {
        return false;
    }
    ctx->scenes = new_scenes;
    ctx->scenes[ctx->n_scenes++] = scene;
    hash_map_insert(ctx->scenes_map, scene->id, scene);

    return true;
}

ui_scene_ctx *init_scene_ctx(ui_scene_ctx *ctx)
{
    if (ctx == NULL)
    {
        ctx = malloc(1 * sizeof(ui_scene_ctx));
        if (!ctx)
            return NULL;
    }

    ctx->button_map = init_hashmap(20);
    ctx->buttons = NULL;
    ctx->n_buttons = 0;

    ctx->n_inputs = 0;
    ctx->inputs = NULL;
    ctx->focused_reader = NULL;

    ctx->n_selectors = 0;
    ctx->selectors = NULL;

    return ctx;
}

void on_input_click(scene_t *scene, ui_input *input)
{
    if (scene == NULL || scene->ctx == NULL || input == NULL)
        return;
    scene->ctx->focused_reader = input;

    int y, x;
    struct ncplane *plane = ncreader_plane(input->reader);
    ncplane_abs_yx(plane, &y, &x);
    notcurses_cursor_enable(scene->nc, y, x);
}

static ui_selector *find_visible_selector(ui_scene_ctx *ctx)
{
    if (ctx == NULL)
        return NULL;
    ui_selector *found = NULL;
    for (size_t i = 0; i < ctx->n_selectors; i++)
    {
        ui_selector *s = ctx->selectors[i];
        if (s != NULL && s->visible && s->selector != NULL)
            found = s;
    }
    return found;
}

void handle_scene_input(scene_t *self, const ncinput *ni, uint32_t key)
{
    if (self == NULL || ni == NULL || self->ctx == NULL)
        return;

    bool is_press =
        (ni->evtype == NCTYPE_PRESS || ni->evtype == NCTYPE_UNKNOWN);
    bool is_release = (ni->evtype == NCTYPE_RELEASE);
    bool is_repeat = (ni->evtype == NCTYPE_REPEAT);

    if (nckey_mouse_p(key))
    {
        if (key == NCKEY_BUTTON1 && is_press)
        {
            if (self->ctx->buttons != NULL
                && handle_click_buttons(self->ctx->buttons,
                                        self->ctx->n_buttons, ni->x, ni->y))
                return;
            if (self->ctx->inputs != NULL
                && handle_click_inputs(self, self->ctx->inputs,
                                       self->ctx->n_inputs, ni->x, ni->y,
                                       on_input_click))
                return;
        }

        if ((key == NCKEY_BUTTON4 || key == NCKEY_BUTTON5) && is_press)
        { // Scroll
            ui_selector *vis = find_visible_selector(self->ctx);
            if (vis != NULL)
                ncselector_offer_input(vis->selector, ni);
        }
        return;
    }

    if (is_release)
        return;

    ui_input *active = self->ctx->focused_reader;

    if (key == NCKEY_ENTER && !is_repeat)
    {
        ui_selector *vis = find_visible_selector(self->ctx);
        if (vis != NULL && vis->opts != NULL && vis->opts->onselect)
            vis->opts->onselect(vis, vis->opts->onselect_payload);
        else if (active != NULL && active->reader != NULL
                 && active->opts != NULL && active->opts->onsubmit)
            active->opts->onsubmit(active, active->opts->onsubmit_payload);
        return;
    }

    if (key == NCKEY_ESC && !is_repeat)
    {
        if (self->ctx->popup != NULL)
        {
            destroy_popup(&self->ctx->popup);
        }
        else
        {
            ui_selector *vis = find_visible_selector(self->ctx);
            if (vis != NULL)
            {
                if (vis->opts->onescape == NULL)
                    hide_selector(vis);
                else
                    vis->opts->onescape(vis, vis->opts->onescape_payload);
            }
        }
        return;
    }

    if (key == NCKEY_TAB && !is_repeat)
    {
        ui_selector *vis = find_visible_selector(self->ctx);
        if (vis != NULL)
            hide_selector(vis);
        return;
    }

    if (key == NCKEY_UP || key == NCKEY_DOWN || key == NCKEY_PGUP
        || key == NCKEY_PGDOWN)
    {
        ui_selector *vis = find_visible_selector(self->ctx);
        if (vis != NULL && vis->opts->is_scrollable == true)
            ncselector_offer_input(vis->selector, ni);
        return;
    }

    /* text editing: only when a reader is focused */
    if (active == NULL || active->reader == NULL || active->opts == NULL)
        return;

    bool is_text = (key < PRETERUNICODEBASE && key >= 0x20);
    if (is_text || key == NCKEY_BACKSPACE || key == NCKEY_DEL)
    {
        ncreader_offer_input(active->reader, ni);
        if (active->opts->ontype != NULL)
            active->opts->ontype(active, active->opts->ontype_payload);
    }
}

void on_scene_enter(scene_t *scene, ncplane_options *nopts)
{
    if (scene == NULL || scene->ctx == NULL || scene->parent == NULL)
    {
        return;
    }

    scene->ctx = init_scene_ctx(scene->ctx);

    struct ncplane *plane = ncplane_create(scene->parent, nopts);
    set_bg_color(plane, 0);
    if (plane == NULL)
        return;
    scene->plane = plane;
}

void on_scene_exit(struct scene *scene)
{
    if (scene == NULL)
        return;

    if (scene->ctx != NULL)
    {
        if (scene->ctx->button_map != NULL)
            free_hashmap(scene->ctx->button_map);
        if (scene->ctx->buttons != NULL)
        {
            for (size_t i = 0; i < scene->ctx->n_buttons; i++)
            {
                free_button(scene->ctx->buttons[i]);
            }
            free(scene->ctx->buttons);
            scene->ctx->buttons = NULL;
            scene->ctx->n_buttons = 0;
        }
        if (scene->ctx->inputs != NULL)
        {
            for (size_t i = 0; i < scene->ctx->n_inputs; i++)
            {
                if (scene->ctx->inputs[i])
                    free_input(scene->ctx->inputs[i]);
            }
            scene->ctx->focused_reader = NULL;
            free(scene->ctx->inputs);
            scene->ctx->inputs = NULL;
            scene->ctx->n_inputs = 0;
        }
        if (scene->ctx->selectors != NULL)
        {
            for (size_t i = 0; i < scene->ctx->n_selectors; i++)
            {
                if (scene->ctx->selectors[i])
                    free_selector(scene->ctx->selectors[i]);
            }
            scene->ctx->focused_reader = NULL;
            free(scene->ctx->selectors);
            scene->ctx->selectors = NULL;
            scene->ctx->n_selectors = 0;
        }
    }
    ncplane_family_destroy(scene->plane);
    scene->plane = NULL;
}

bool switch_scene(ui_ctx *ctx, scene_t *new)
{
    if (ctx == NULL)
        return false;

    if (ctx->curr_scene != NULL && ctx->curr_scene->on_exit != NULL)
        ctx->curr_scene->on_exit(ctx->curr_scene);
    if (new != NULL &&new->on_enter != NULL)
    {
        new->on_enter(new);
    }
    ctx->curr_scene = new;
    return true;
}

void destroy_scene(struct scene *scene)
{
    if (scene)
    {
        if (scene->ctx && scene->ctx->button_map)
            free_hashmap(scene->ctx->button_map);
        free(scene->ctx);
        free(scene->payload);
        free(scene->id);
        free(scene);
    }
}

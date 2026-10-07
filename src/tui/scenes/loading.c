#include <stdlib.h>
#include <stdbool.h>
#include <notcurses/notcurses.h>
#include "tui/ui/button.h"
#include "tui/scenes/scenes.h"

void scene_loading_on_enter(scene_t *scene)
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
                                         .name = "scene_loading_plane" };
        on_scene_enter(scene, &nopts);
        ncplane_set_scrolling(scene->plane, true);
        return;
}

scene_t *create_loading_scene(struct notcurses *nc,
                              struct ncplane *parent_plane, ui_ctx *parent_ctx)
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

        scene->type = UI_LOADING_SCENE;
        scene->ctx = ctx;
        scene->parent_ctx = parent_ctx;
        scene->parent = parent_plane;
        scene->payload = NULL;
        scene->handle_input = handle_scene_input;
        scene->on_enter = scene_loading_on_enter;
        scene->on_exit = on_scene_exit;
        scene->id = strdup("loading_scene");
        scene->nc = nc;

        return scene;
}

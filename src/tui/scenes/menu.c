#include "tui/utils.h"
#include <stdlib.h>
#include <stdbool.h>
#include <notcurses/notcurses.h>
#include "tui/ui/button.h"
#include "tui/scenes/scenes.h"
#include "tui/scenes/itinerary.h"
#include <string.h>
#include "tui/scenes/menu.h"

typedef struct {
        scene_t *self;
} menu_button_switch_scene_payload;

void menu_button_itinerary(void *data)
{
        if (data == NULL)
                return;

        scene_t *scene = data;
        if (scene == NULL || scene->parent_ctx == NULL)
                return;
        scene_t *itinerary_scene = NULL;
        for (size_t i = 0; i < scene->parent_ctx->n_scenes; i++) {
                scene_t *s = scene->parent_ctx->scenes[i];
                if (s && s->id && strcmp(s->id, "itinerary_scene") == 0)
                        itinerary_scene = s;
        }

        if (itinerary_scene == NULL) {
                itinerary_scene = create_itinerary_scene(
                        scene->nc, scene->parent, scene->parent_ctx);
                add_scene(itinerary_scene, scene->parent_ctx);
        }
        switch_scene(scene->parent_ctx, itinerary_scene);
}

void scene_menu_on_enter(scene_t *scene)
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
                                         .name = "scene_menu_plane" };
        on_scene_enter(scene, &nopts);

        const char *title_art[] = {
                " ___ ____  _____ __  __   _____ _   _ ___ ",
                "|_ _|  _ \\|  ___|  \\/  | |_   _| | | |_ _|",
                " | || | | | |_  | |\\/| |   | | | | | || | ",
                " | || |_| |  _| | |  | |   | | | |_| || | ",
                "|___|____/|_|   |_|  |_|   |_|  \\___/|___|"
        };

        // Allocate a small sub-plane for the header title
        struct ncplane_options opts = {
                .y = 2,
                .x = NCALIGN_CENTER, // Center align in parent
                .rows = 30,
                .cols = 52,
                .flags = NCPLANE_OPTION_HORALIGNED
        };

        struct ncplane *title_plane = ncplane_create(scene->plane, &opts);

        // Apply bright yellow text styling
        uint64_t channels = 0;

        set_bg_color(title_plane, channels);

        // Print lines
        for (int i = 0; i < 5; i++) {
                ncplane_printf_aligned(title_plane, i, NCALIGN_CENTER, "%s",
                                       title_art[i]);
        }

        ui_button_opts btn_opts = {
                .h = 3,
                .w = 20,
                .x = cols / 2 - 10,
                .y = 10,
                .id = "itinerary_button",
                .onclick = menu_button_itinerary,
                .onclick_payload = scene,
        };

        add_button(scene->plane, &scene->ctx->buttons, &scene->ctx->n_buttons,
                   scene->ctx->button_map, &btn_opts, "Itinerary Search");
        return;
}

scene_t *create_menu_scene(struct notcurses *nc, struct ncplane *parent_plane,
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

        scene->type = UI_MENU_SCENE;
        scene->ctx = ctx;
        scene->parent_ctx = parent_ctx;
        scene->parent = parent_plane;
        scene->payload = NULL;
        scene->handle_input = handle_scene_input;
        scene->on_enter = scene_menu_on_enter;
        scene->on_exit = on_scene_exit;
        scene->id = strdup("menu_scene");
        scene->nc = nc;

        return scene;
}

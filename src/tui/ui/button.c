#include <notcurses/notcurses.h>
#include "tui/ui/button.h"
#include "utils/hashmap.h"
#include "tui/utils.h"
#include <stdlib.h>
#include <stdio.h>

void resize_btn(ui_button *btn, size_t h, size_t w)
{
        if (!btn || btn->plane == NULL)
                return;
        ncplane_resize_simple(btn->plane, h, w);
}
void move_btn(ui_button *btn, int x, int y)
{
        if (!btn || btn->plane == NULL)
                return;
        ncplane_move_yx(btn->plane, x, y);
}

ui_button *create_button(struct ncplane *parent, ui_button_opts *opts,
                         const char *fmt, va_list args)
{
        if (opts == NULL || opts->id == NULL)
                return NULL;
        struct ncplane_options nopts = {
                .x = opts->x,
                .y = opts->y,
                .rows = opts->h, // Height (lines)
                .cols = opts->w, // Width (columns)
                .userptr = NULL, // Optional user data pointer
                .name = "button" // Optional string label for debugging
        };
        struct ncplane *btn_plane = ncplane_create(parent, &nopts);
        uint64_t channels = 0;
        set_bg_color(btn_plane, channels);

        if (!btn_plane)
                return NULL;

        ncplane_perimeter_rounded(btn_plane, 0, channels, 0);

        char *text;

        if (vasprintf(&text, fmt, args) != -1) {
                ncplane_printf_aligned(btn_plane, opts->h / 2, NCALIGN_CENTER,
                                       "%s", text);
                free(text);
        }

        ui_button *btn = malloc(1 * sizeof(ui_button));
        ui_button_opts *heap_opts = malloc(sizeof(ui_button_opts));
        if (!btn || !heap_opts) {
                free(btn);
                free(heap_opts);
                ncplane_destroy(btn_plane);
                return NULL;
        }
        *heap_opts = *opts;
        heap_opts->id = strdup(opts->id);
        btn->opts = heap_opts;
        btn->plane = btn_plane;
        btn->parent = parent;

        return btn;
}

void add_button(struct ncplane *parent, ui_button ***button_array,
                size_t *button_n, struct hash_map *button_map,
                ui_button_opts *opts, const char *fmt, ...)
{
        if (button_array == NULL || parent == NULL || button_map == NULL ||
            opts == NULL)
                return;

        va_list arglist;
        va_start(arglist, fmt);

        ui_button *btn = create_button(parent, opts, fmt, arglist);
        va_end(arglist);
        if (!btn)
                return;

        ui_button **new_array =
                realloc(*button_array, (*button_n + 1) * sizeof(ui_button *));

        if (!new_array) {
                free_button(btn);
                return;
        }

        *button_array = new_array;
        (*button_array)[*button_n] = btn;
        (*button_n)++;

        hash_map_insert(button_map, opts->id, btn);
}

void free_button(ui_button *btn)
{
        if (btn == NULL)
                return;
        if (btn->opts != NULL) {
                free(btn->opts->id);
                free(btn->opts);
        }
        ncplane_destroy(btn->plane);
        free(btn);
}

#include "tui/ui/selector.h"

#include <errno.h>
#include <notcurses/notcurses.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "tui/ui/input.h"
#include "tui/ui/utils.h"
#include "tui/utils.h"

struct ncselector *create_nc_selector(struct ncplane *box_plane,
                                      ui_selector_opts *opts)
{
    struct ncplane_options selector_plane_opts = {
        .y = 1, .x = 1, .rows = opts->rows - 2, .cols = opts->cols - 2
    };
    struct ncplane *selector_plane =
        ncplane_create(box_plane, &selector_plane_opts);

    uint64_t text_ch = 0;
    ncchannels_set_fg_rgb(&text_ch, RATP_BEIGE_WHITE); // White text
    ncchannels_set_bg_rgb(&text_ch,
                          RATP_DEEP_BLUE); // Dark blue background
    set_bg_color(selector_plane, text_ch);

    uint64_t border_ch = 0;

    struct ncselector_options sopts = { .opchannels = text_ch,
                                        .boxchannels = border_ch };
    struct ncselector *selector = ncselector_create(selector_plane, &sopts);
    if (selector == NULL)
        ncplane_destroy(selector_plane);
    return selector;
}

struct ncplane *create_box_plane(struct ncplane *parent, ui_selector_opts *opts)
{
    struct ncplane_options box_opts = {
        .y = opts->y, .x = opts->x, .rows = opts->rows, .cols = opts->cols
    };
    struct ncplane *box_plane = ncplane_create(parent, &box_opts);

    uint64_t box_ch = 0;
    ncchannels_set_bg_rgb(&box_ch, RATP_DEEP_BLUE);
    set_bg_color(box_plane, box_ch);

    /*nccell ul = NCCELL_CHAR_INITIALIZER('+');
    nccell hl = NCCELL_CHAR_INITIALIZER('-');
    nccell vl = NCCELL_CHAR_INITIALIZER('|');
    ncplane_perimeter(box_plane, &ul, &ul, &ul, &ul, &hl, &vl, 0);*/
    return box_plane;
}

ui_selector *create_ui_selector(struct ncplane *parent,
                                struct ui_input *parent_input,
                                ui_selector_opts *selector_opts)
{
    if (parent == NULL || selector_opts == NULL || selector_opts->rows < 3
        || selector_opts->cols < 3)
    {
        errno = EINVAL;
        return NULL;
    }

    ui_selector_opts *opts = malloc(sizeof(ui_selector_opts));
    if (opts == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }
    *opts = *selector_opts;
    opts->id = opts->id ? strdup(opts->id) : NULL;

    struct ncplane *box_plane = create_box_plane(parent, opts);
    // 2. Reader plane inside the box (y=1, x=1 so it stays inside the borders)
    struct ncselector *selector = create_nc_selector(box_plane, selector_opts);
    if (selector == NULL)
    {
        free(opts->id);
        free(opts);
        ncplane_destroy(box_plane);
        return NULL;
    }
    ui_selector *sel = calloc(1, sizeof(ui_selector));
    if (sel == NULL)
    {
        free(opts->id);
        free(opts);
        ncplane_destroy(box_plane);
        ncselector_destroy(selector, NULL);
        errno = ENOMEM;
        return NULL;
    }
    sel->opts = opts;
    sel->box_plane = box_plane;
    sel->selector = selector;
    sel->parent_input = parent_input;
    sel->parent_plane = parent;
    sel->visible = true;
    return sel;
}

ui_selector *add_selector(struct ncplane *parent, struct ui_input *parent_input,
                          ui_selector ***selector_array, size_t *selector_n,
                          ui_selector_opts *opts, bool visible)
{
    if (selector_array == NULL || parent == NULL || opts == NULL)
        return NULL;

    ui_selector *ipt;
    if (visible)
    {
        ipt = create_ui_selector(parent, parent_input, opts);
        if (!ipt)
            return NULL;
    }
    else
    {
        ipt = calloc(1, sizeof(ui_selector));
        if (ipt == NULL)
            return NULL;
        ui_selector_opts *h_opts = malloc(sizeof(ui_selector_opts));
        if (h_opts == NULL)
        {
            free(ipt);
            return NULL;
        }
        *h_opts = *opts;
        h_opts->id = opts->id ? strdup(opts->id) : NULL;
        ipt->parent_plane = parent;
        ipt->parent_input = parent_input;
        ipt->opts = h_opts;
        ipt->visible = false;
        ipt->items = NULL;
        ipt->n_items = 0;
    }

    ui_selector **new_array =
        realloc(*selector_array, (*selector_n + 1) * sizeof(ui_selector *));

    if (!new_array)
    {
        free_selector(ipt);
        return NULL;
    }

    *selector_array = new_array;
    (*selector_array)[*selector_n] = ipt;
    (*selector_n)++;
    return ipt;
}

int uiselector_add_item(ui_selector *sel, struct ncselector_item *item,
                        void *orig_item)
{
    if (sel == NULL || sel->selector == NULL || item == NULL
        || (sel->items == NULL && sel->n_items > 0))
        return 1;
    struct ncselector_item **t_items = realloc(
        sel->items, (sel->n_items + 1) * sizeof(struct ncselector_item *));
    void **t_orig_items =
        realloc(sel->orig_items, (sel->n_items + 1) * sizeof(void *));
    if (t_items == NULL || t_orig_items == NULL)
        return 1;
    sel->orig_items = t_orig_items;
    sel->items = t_items;
    sel->orig_items[sel->n_items] = orig_item;
    sel->items[sel->n_items++] = item;
    ncselector_additem(sel->selector, item);
    return 0;
}

ui_selector *show_selector(ui_selector *ui_sel)
{
    if (ui_sel == NULL)
        return NULL;
    struct ncplane *box_plane =
        create_box_plane(ui_sel->parent_plane, ui_sel->opts);
    ui_sel->box_plane = box_plane;
    ui_sel->selector = create_nc_selector(ui_sel->box_plane, ui_sel->opts);
    ui_sel->visible = true;
    return ui_sel;
}

ui_selector *hide_selector(ui_selector *ui_sel)
{
    if (ui_sel == NULL)
        return NULL;
    ncselector_destroy(ui_sel->selector, NULL);
    ncplane_destroy(ui_sel->box_plane);
    ui_sel->selector = NULL;
    ui_sel->box_plane = NULL;
    ui_sel->visible = false;
    free_selector_items(&ui_sel->items, &ui_sel->n_items);
    free(ui_sel->orig_items);
    ui_sel->orig_items = NULL;
    return ui_sel;
}

ui_selector *reset_selector(ui_selector *ui_sel)
{
    if (ui_sel == NULL)
        return NULL;
    ui_sel = hide_selector(ui_sel);
    return show_selector(ui_sel);
}

void free_selector(ui_selector *ipt)
{
    if (ipt == NULL)
        return;
    if (ipt->opts != NULL)
    {
        free(ipt->opts->id);
        free(ipt->opts);
    }
    ncselector_destroy(ipt->selector, NULL);
    ncplane_destroy(ipt->box_plane);
    free_selector_items(&ipt->items, &ipt->n_items);
    free(ipt->orig_items);
    free(ipt);
}

void free_selector_item(struct ncselector_item *item)
{
    if (item == NULL)
        return;
    free((void *)item->desc);
    free((void *)item->option);
    free(item);
}

void free_selector_items(struct ncselector_item ***items, size_t *n_items)
{
    if (items == NULL || *items == NULL)
        return;
    for (size_t i = 0; i < *n_items; i++)
    {
        free_selector_item((*items)[i]);
    }
    free(*items);
    *items = NULL;
    *n_items = 0;
}

struct ncselector_item *clone_selector_item(const struct ncselector_item *src)
{
    if (src == NULL)
        return NULL;
    struct ncselector_item *c = calloc(1, sizeof *c);
    if (c == NULL)
        return NULL;
    if (src->option != NULL)
        c->option = strdup(src->option);
    if (src->desc != NULL)
        c->desc = strdup(src->desc);
    return c;
}

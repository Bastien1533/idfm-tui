#ifndef SELECTOR_H
#define SELECTOR_H

#include <notcurses/notcurses.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct ui_input ui_input;
typedef struct ui_selector ui_selector;

typedef struct
{
    int x;
    int y;
    size_t cols;
    size_t rows;

    char *id;

    void (*onselect)(ui_selector *sel, void *arg);
    void *onselect_payload;

    void (*onescape)(ui_selector *sel, void *arg);
    void *onescape_payload;

    bool is_scrollable;
} ui_selector_opts;

typedef struct ui_selector
{
    struct ncselector *selector;
    struct ncplane *box_plane;
    struct ncplane *parent_plane;

    struct ncselector_item **items;
    void **orig_items;
    size_t n_items;

    ui_selector_opts *opts;
    ui_input *parent_input;

    size_t pending_seq;

    bool visible;
} ui_selector;

// ui_selector *create_selector(struct ncplane *parent, ui_selector_opts
// *selector_opts);
ui_selector *add_selector(struct ncplane *parent, struct ui_input *parent_input,
                          ui_selector ***selector_array, size_t *selector_n,
                          ui_selector_opts *opts, bool visible);
void reset(ui_selector *selector);
void free_selector(ui_selector *selector);
struct ncselector_item *clone_selector_item(const struct ncselector_item *src);
void free_selector_item(struct ncselector_item *item);
void free_selector_items(struct ncselector_item ***items, size_t *n_items);
int uiselector_add_item(ui_selector *sel, struct ncselector_item *item,
                        void *orig_item);
ui_selector *reset_selector(ui_selector *ui_sel);
ui_selector *show_selector(ui_selector *ui_sel);
ui_selector *hide_selector(ui_selector *ui_sel);

#endif

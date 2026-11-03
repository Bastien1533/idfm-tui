#ifndef BTN_SELECTOR_UI_H
#define BTN_SELECTOR_UI_H

#include <notcurses/notcurses.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef struct ui_btn_selector_item {
        char *id;
        char *desc;
} ui_btn_selector_item;

typedef struct ui_btn_selector_opts {
        int x;
        int y;

        size_t rows;
        size_t cols;

        char *id;

        size_t pad;
        size_t n_items;
        ui_btn_selector_item *items;

        void (*onselect)(char *id, void *arg);
        void *onselect_payload;

        uint64_t selection_bitmask;
        int64_t hovered;
        bool is_radio;
} ui_btn_selector_opts;

typedef struct ui_btn_selector {
        struct ncplane **item_planes;
        struct ncplane *plane;
        struct ncplane *parent;
        ui_btn_selector_opts *opts;
} ui_btn_selector;

void get_btn_selector_size(ui_btn_selector_item *items, size_t n_items,
                           size_t pad, size_t *rows, size_t *cols);

void toggle_item(ui_btn_selector *sel, size_t idx);
bool is_selected_item(ui_btn_selector *sel, size_t idx);
void set_selector_focused(ui_btn_selector *sel);
void set_selector_unfocused(ui_btn_selector *sel);

void handle_btn_selector_input(ui_btn_selector *sel, const ncinput *ni,
                               uint32_t key);
ui_btn_selector *create_btn_selector(struct ncplane *parent,
                                     ui_btn_selector_opts *opts);
void free_btn_selector(ui_btn_selector *btn);

#endif

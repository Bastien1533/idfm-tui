#include <assert.h>
#include <notcurses/notcurses.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "utils/utils.h"
#include "tui/utils.h"
#include "button_selector.h"

/*int nx, ny, ox, oy;
get_item_str_pos(sel, n_idx, &nx, &ny);
get_item_str_pos(sel, o_idx, &ox, &oy);

char* o_str;
char* n_str;
get_item_str(sel->opts->items[n_idx], is_selected_item(sel, o_idx), false, sel->opts->is_radio, &n_str);
get_item_str(sel->opts->items[o_idx], is_selected_item(sel, o_idx), false, sel->opts->is_radio, &o_str);

ncplane_putstr_yx(sel->plane, ny, nx, n_str);
ncplane_putstr_yx(sel->plane, oy, ox, o_str);
free(n_str);
free(o_str); */

static void get_item_str(ui_btn_selector_item item, bool selected, bool hovered,
                         bool is_radio, char **dst)
{
        char *hov_s = hovered ? "►" : " ";
        char *sel_s = selected ? (is_radio ? "(•)" : "[x]") :
                                 (is_radio ? "( )" : "[ ]");

        asprintf(dst, " %s %s %s ", hov_s, item.desc, sel_s);
}

/*static void get_item_str_pos_rel(ui_btn_selector *sel, size_t idx, int *x, int *y)
{
        // x,y is the top_left.
        *x = sel->opts->x + (sel->opts->pad) + 2;
        *y = sel->opts->y + (idx * 3);
} */

static void get_item_str_pos(ui_btn_selector *sel, size_t idx, int *x, int *y)
{
        // x,y is the top_left.
        *x = (sel->opts->pad) + 2;
        *y = 1 + (idx * 3);
}

static void draw_item(ui_btn_selector *sel, size_t idx)
{
        int nx, ny;
        get_item_str_pos(sel, idx, &nx, &ny);

        char *n_str;
        get_item_str(sel->opts->items[idx], is_selected_item(sel, idx),
                     (size_t)sel->opts->hovered == idx, sel->opts->is_radio,
                     &n_str);

        ncplane_putstr_yx(sel->plane, ny, nx, n_str);
        free(n_str);
}

static void draw(ui_btn_selector *sel)
{
        for (size_t i = 0; i < sel->opts->n_items; i++) {
                draw_item(sel, i);
        }
}

void get_btn_selector_size(ui_btn_selector_item *items, size_t n_items,
                           size_t pad, size_t *rows, size_t *cols)
{
        size_t max_len = 9 + strlen(items[0].desc);
        for (size_t i = 1; i < n_items; i++) {
                size_t len = strlen(items[i].desc);
                max_len = max_len < 9 + len ? 9 + len : max_len;
        }
        *cols = 2 + (max_len + pad * 2); //2 for borders
        *rows = 2 + (n_items * 3); //2 for borders
}

void toggle_item(ui_btn_selector *sel, size_t idx)
{
        if (sel->opts->is_radio) {
                sel->opts->selection_bitmask = (1 << idx);
        } else
                sel->opts->selection_bitmask ^= (1 << idx);

        for (size_t i = 0; i < sel->opts->n_items; i++) {
                draw_item(sel, i); //TODO: choose which to redraw.
        }
        if (sel->opts->onselect && sel->opts->onselect_payload)
                sel->opts->onselect(sel->opts->items[idx].id,
                                    sel->opts->onselect_payload);
}

bool is_selected_item(ui_btn_selector *sel, size_t idx)
{
        return ((sel->opts->selection_bitmask >> idx) & 1) == 1;
}

bool change_hovered(ui_btn_selector *sel, long offy)
{
        int64_t o_idx = sel->opts->hovered;
        size_t n_idx = stmod(sel->opts->hovered + offy, sel->opts->n_items);

        sel->opts->hovered = n_idx;

        if (o_idx != -1)
                draw_item(sel, o_idx);
        draw_item(sel, n_idx);
        return true;
}

void set_selector_focused(ui_btn_selector *sel)
{
        uint64_t channels = 0;
        set_bg_color(sel->plane, channels);
        uint64_t border_ch = channels;
        ncchannels_set_fg_rgb(&border_ch, 0x4bc0ad);
        ncplane_perimeter_rounded(sel->plane, 0, border_ch, 0);
}

void set_selector_unfocused(ui_btn_selector *sel)
{
        uint64_t channels = 0;
        set_bg_color(sel->plane, channels);
        uint64_t border_ch = channels;
        ncplane_perimeter_rounded(sel->plane, 0, border_ch, 0);
}

void handle_btn_selector_input(ui_btn_selector *sel, const ncinput *ni,
                               uint32_t key)
{
        if (ni == NULL)
                return;

        bool is_press =
                (ni->evtype == NCTYPE_PRESS || ni->evtype == NCTYPE_UNKNOWN);
        bool is_release = (ni->evtype == NCTYPE_RELEASE);
        bool is_repeat = (ni->evtype == NCTYPE_REPEAT);

        if (is_release)
                return;

        if (key == NCKEY_UP) {
                change_hovered(sel, -1);
                return;
        }
        if (key == NCKEY_DOWN) {
                change_hovered(sel, 1);
                return;
        }
        if (key == NCKEY_ENTER && sel->opts->hovered != -1) {
                toggle_item(sel, sel->opts->hovered);
                return;
        }
}
ui_btn_selector *create_btn_selector(struct ncplane *parent,
                                     ui_btn_selector_opts *opts)
{
        if (opts == NULL || opts->id == NULL || parent == NULL)
                return NULL;
        size_t r, c;
        get_btn_selector_size(opts->items, opts->n_items, opts->pad, &r, &c);
        struct ncplane_options nopts = {
                .x = opts->x,
                .y = opts->y,
                .rows = r, // Height (lines)
                .cols = c, // Width (columns)
                .userptr = NULL, // Optional user data pointer
                .name = "selector" // Optional string label for debugging
        };
        struct ncplane *btn_sel_plane = ncplane_create(parent, &nopts);

        if (!btn_sel_plane)
                return NULL;

        uint64_t channels = 0;
        set_bg_color(btn_sel_plane, channels);
        uint64_t border_ch = channels;
        ncplane_perimeter_rounded(btn_sel_plane, 0, border_ch, 0);

        ui_btn_selector *btn_sel = malloc(1 * sizeof(ui_btn_selector));
        ui_btn_selector_opts *heap_opts = malloc(sizeof(ui_btn_selector_opts));
        if (!btn_sel || !heap_opts) {
                free(btn_sel);
                free(heap_opts);
                ncplane_destroy(btn_sel_plane);
                return NULL;
        }
        *heap_opts = *opts;
        heap_opts->id = strdup(opts->id);
        heap_opts->hovered = -1;
        btn_sel->opts = heap_opts;
        btn_sel->plane = btn_sel_plane;
        btn_sel->parent = parent;
        btn_sel->opts->rows = r;
        btn_sel->opts->cols = c;

        draw(btn_sel);

        return btn_sel;
}
void free_btn_selector(ui_btn_selector *btn)
{
        free(btn->opts->id);
        free(btn->opts->items);
        free(btn->opts);
        ncplane_destroy(btn->plane);
        free(btn);
}

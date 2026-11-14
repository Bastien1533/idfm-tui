#ifndef BTN_UI_H
#define BTN_UI_H

#include <notcurses/notcurses.h>
#include <stdarg.h>

#include "utils/hashmap.h"

typedef struct
{
    int x;
    int y;
    size_t w;
    size_t h;

    char *id;

    void (*onclick)(void *arg);
    void *onclick_payload;

    void (*onhover)(void *arg);
    void *onhover_payload;
} ui_button_opts;

typedef struct ui_button
{
    struct ncplane *plane;
    struct ncplane *parent;
    ui_button_opts *opts;
} ui_button;

void resize_btn(ui_button *btn, size_t h, size_t w);
void move_btn(ui_button *btn, int x, int y);
ui_button *create_button(struct ncplane *parent, ui_button_opts *opts,
                         const char *fmt, va_list args);
void add_button(struct ncplane *parent, ui_button ***button_array,
                size_t *button_n, struct hash_map *button_map,
                ui_button_opts *opts, const char *fmt, ...);
void free_button(ui_button *btn);

#endif

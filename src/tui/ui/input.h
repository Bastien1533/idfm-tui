#ifndef INPUT_H
#define INPUT_H

#include "tui/ui/selector.h"
#include <notcurses/notcurses.h>
#include <stdarg.h>

struct scene;
typedef struct ui_input ui_input;

typedef struct {
        int x;
        int y;
        size_t cols;
        size_t rows;

        char *id;

        void (*onsubmit)(ui_input *ipt, void *arg);
        void *onsubmit_payload;

        void (*ontype)(ui_input *ipt, void *arg);
        void *ontype_payload;
} ui_input_opts;

typedef struct ui_input {
        struct ncreader *reader;
        struct ncplane *box_plane;
        ui_input_opts *opts;
        struct ui_selector *child_selector;
} ui_input;

void move_input(ui_input *btn, int x, int y);
ui_input *create_input(struct ncplane *parent, ui_input_opts *input_opts);
ui_input *add_input(struct ncplane *parent, ui_input ***input_array,
                    size_t *input_n, ui_input_opts *opts);
void free_input(ui_input *btn);

#endif

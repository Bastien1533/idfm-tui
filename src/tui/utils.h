#ifndef TUI_UTILS_H
#define TUI_UTILS_H

#include "tui/tui.h"
#include "tui/scenes/scenes.h"
#include <stdbool.h>
#include <notcurses/notcurses.h>

void quit_app(ui_ctx *ctx, struct ncplane *child_plane, struct notcurses *nc);
void set_bg_color(struct ncplane *plane, uint64_t ch);

#endif

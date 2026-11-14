#include "tui/utils.h"

#include <notcurses/notcurses.h>
#include <stdbool.h>

#include "tui/scenes/scenes.h"
#include "tui/ui/utils.h"
#include "utils/hashmap.h"

void quit_app(ui_ctx *ctx, struct ncplane *child_plane, struct notcurses *nc)
{
    switch_scene(ctx, NULL);

    for (size_t i = 0; i < ctx->n_scenes; i++)
    {
        destroy_scene(ctx->scenes[i]);
    }
    free(ctx->scenes);
    free_hashmap(ctx->scenes_map);
    free(ctx);
    ncplane_destroy(child_plane);
    notcurses_stop(nc);
}

static bool fg_defined(uint64_t ch)
{
    return ncchannels_fg_rgb_p(ch) || ncchannels_fg_palindex_p(ch);
}
static bool bg_defined(uint64_t ch)
{
    return ncchannels_bg_rgb_p(ch) || ncchannels_bg_palindex_p(ch);
}

void set_bg_color(struct ncplane *plane, uint64_t ch)
{
    if (!bg_defined(ch))
    {
        ncchannels_set_bg_rgb(&ch, RATP_BEIGE_WHITE);
        ncchannels_set_bg_alpha(&ch, NCALPHA_OPAQUE);
    }
    if (!fg_defined(ch))
        ncchannels_set_fg_rgb(&ch, RATP_DEEP_BLUE);
    ncplane_set_base(plane, " ", 0, ch);
}

#include "tui/ui/popups/popups.h"

#include <notcurses/notcurses.h>
#include <stdint.h>

#include "tui/scenes/scenes.h"
#include "tui/utils.h"

void destroy_popup(ui_popup **popup)
{
    if (popup == NULL || *popup == NULL)
        return;
    ncplane_destroy((*popup)->plane);
    free((*popup)->title);
    free((*popup)->id);
    free((*popup));
    *popup = NULL;
}
struct ncplane *get_popup_plane(ui_popup *popup, ncplane_options *nc_opts)
{
    if (popup == NULL || popup->parent_scene == NULL
        || popup->parent_scene->ctx == NULL)
    {
        return NULL;
    }
    if (popup->parent_scene->ctx->popup != NULL)
        destroy_popup(&popup->parent_scene->ctx->popup);

    struct ncplane *plane = ncplane_create(popup->parent_scene->plane, nc_opts);
    if (plane == NULL)
        return NULL;
    uint64_t ch = 0;
    set_bg_color(plane, ch);
    ncplane_perimeter_rounded(plane, 0, ch, 0);
    popup->parent_scene->ctx->popup = popup;
    return plane;
}

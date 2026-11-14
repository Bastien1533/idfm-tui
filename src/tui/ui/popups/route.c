#include <stdlib.h>
#include <string.h>

#include "data/gtfs.h"
#include "tui/scenes/scenes.h"
#include "tui/ui/popups/popups.h"

ui_popup *create_route_popup(scene_t *scene, struct route *route)
{
    if (scene == NULL || scene->plane == NULL || route == NULL)
        return NULL;

    ui_popup *popup = calloc(1, sizeof(ui_popup));
    if (popup == NULL)
        return NULL;

    popup->parent_scene = scene;
    popup->id = strdup("Route Popup");

    char *title;
    int stat = asprintf(&title, "Info for Route: %s (%s)",
                        route->route_short_name, route->route_id);
    if (stat == -1)
    {
        free(popup->id);
        free(popup);
        return NULL;
    }
    popup->title = title;

    unsigned rows, cols;
    ncplane_dim_yx(scene->parent, &rows, &cols);
    unsigned width = cols * 2 / 3;
    unsigned height = rows * 3 / 4;

    struct ncplane_options nc_opts = {
        .y = (rows / 4 / 2), .x = (cols / 3 / 2), .rows = height, .cols = width
    };

    struct ncplane *plane = get_popup_plane(popup, &nc_opts);
    if (plane == NULL)
    {
        destroy_popup(&popup);
        return NULL;
    }
    popup->plane = plane;

    ncplane_putstr_yx(plane, 1,
                      width / 2 - ncstrwidth(popup->title, NULL, NULL) / 2,
                      popup->title);

    char *desc;
    asprintf(&desc, "Terminus: ");
    ncplane_putstr_yx(plane, 5, width / 2 - ncstrwidth(desc, NULL, NULL) / 2,
                      desc);

    for (size_t i = 0; i < route->n_end_stops; i++)
    {
        struct stop *s = route->end_stops[i];
        char *line;
        asprintf(&line, "%s", s->stop_name);
        ncplane_putstr_yx(plane, 5 + (i + 1),
                          width / 2 - ncstrwidth(line, NULL, NULL) / 2, line);
    }

    return popup;
}

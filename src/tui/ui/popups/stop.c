#include "tui/ui/popups/popups.h"
#include "tui/scenes/scenes.h"
#include "data/gtfs.h"
#include <stdlib.h>
#include <string.h>

ui_popup *create_stop_popup(scene_t *scene, struct stop *stop)
{
        if (scene == NULL || scene->plane == NULL || stop == NULL)
                return NULL;

        ui_popup *popup = calloc(1, sizeof(ui_popup));
        if (popup == NULL)
                return NULL;

        popup->parent_scene = scene;
        popup->id = strdup("Stop Popup");

        char *title;
        int stat = asprintf(&title, "Info for stop: %s (%s)", stop->stop_name,
                            stop->stop_id);
        if (stat == -1) {
                free(popup->id);
                free(popup);
                return NULL;
        }
        popup->title = title;

        unsigned rows, cols;
        ncplane_dim_yx(scene->parent, &rows, &cols);
        unsigned width = cols * 2 / 3;
        unsigned height = rows * 3 / 4;

        struct ncplane_options nc_opts = { .y = (rows / 4 / 2),
                                           .x = (cols / 3 / 2),
                                           .rows = height,
                                           .cols = width };

        struct ncplane *plane = get_popup_plane(popup, &nc_opts);
        if (plane == NULL) {
                destroy_popup(&popup);
                return NULL;
        }
        popup->plane = plane;

        ncplane_putstr_yx(plane, 1,
                          width / 2 - ncstrwidth(popup->title, NULL, NULL) / 2,
                          popup->title);

        char *desc;
        asprintf(&desc, "Associated lines: ");
        size_t len = strlen(desc);

        struct stop *pstop =
                (stop->parent_station == NULL) ? stop : stop->parent_station;
        for (size_t i = 0; i < pstop->n_assoc_routes; i++) {
                struct route *r = pstop->assoc_routes[i];
                char *line_str;
                asprintf(&line_str, " %s", r->route_short_name);

                char *desc_t = realloc(desc, (len + 1 + strlen(line_str)) *
                                                     sizeof(char));
                if (desc_t == NULL) {
                        free(line_str);
                        free(desc);
                        return NULL;
                }
                desc = desc_t;
                strlcat(desc, line_str, len + 1 + strlen(line_str));
                len = len + strlen(line_str);
                free(line_str);
        }
        ncplane_putstr_yx(plane, 5,
                          width / 2 - ncstrwidth(desc, NULL, NULL) / 2, desc);
        return popup;
}

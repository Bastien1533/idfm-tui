#include "style.h"
#include "data/rules.h"
#include <stdbool.h>
#include <stdint.h>

uint32_t get_stop_color(enum location_type location_type, enum stop_role role)
{
        uint32_t color;
        switch (location_type) {
        case PLATFORM:
                color = STOP_PLATFORM_COLOR;
                break;
        case STATION:
                color = STOP_STATION_COLOR;
                break;
        case ENTRANCE_EXIT:
                color = STOP_EXIT_COLOR;
                break;
        default:
                color = 0xFFFFF;
                break;
        }
        switch (role) {
        case STOP_ROLE_INTERMEDIATE:
                color = 0xADADAD;
                break;
        case STOP_ROLE_DEP:
                color = 0x63ffd0;
                break;
        case STOP_ROLE_ARR:
                color = 0x63ffd0;
                break;
        case STOP_ROLE_TRANSFER:
                color = 0xffffff;
                break;
        default:
                break;
        }
        return color;
}

void get_stop_min_max_scale(enum location_type location_type,
                            enum stop_role stop_role, int *min_scale,
                            int *max_scale)
{
        int min_scale_t = -1;
        int max_scale_t = -1;
        switch (location_type) {
        case PLATFORM:
                min_scale_t = 300;
                break;
        case STATION:
                min_scale_t = 20;
                break;
        default:
                min_scale_t = 300;
                break;
        }
        if ((stop_role == STOP_ROLE_DEP || stop_role == STOP_ROLE_ARR ||
             stop_role == STOP_ROLE_TRANSFER) &&
            location_type == STATION)
                min_scale_t = -1;
        *min_scale = min_scale_t;
        *max_scale = max_scale_t;
}

struct shape_style style_stop(const struct stop *s)
{
        double radius = s->location_type == STATION ? 2 : 1;
        uint32_t color = get_stop_color(s->location_type, s->stop_role);

        int min_scale;
        int max_scale;
        get_stop_min_max_scale(s->location_type, s->stop_role, &min_scale,
                               &max_scale);
        bool label = s->location_type == STATION ? true : false;
        struct shape_style style = {
                .label = label,
                .color = color,
                .max_scale = max_scale,
                .min_scale = min_scale,
                .radius = radius,
                .z_index = 2,
        };
        return style;
}

struct shape_style style_route(const struct route *r)
{
        uint32_t color = (uint32_t)((r && r->route_color[0]) ?
                                            strtoul(r->route_color, NULL, 16) :
                                            0xFFFFFF);
        struct shape_style style = {
                .label = false,
                .color = color,
                .max_scale = -1,
                .min_scale = -1,
                .radius = -1,
                .z_index = 1,
        };
        return style;
}

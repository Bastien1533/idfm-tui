#include "style.h"
#include "data/rules.h"
#include <stdbool.h>
#include <stdint.h>

struct shape_style style_stop(const struct stop *s)
{
        double radius = s->location_type == STATION ? 2 : 1;
        uint32_t color;
        switch (s->location_type) {
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
        bool label = s->location_type == STATION ? true : false;
        struct shape_style style = {
                .label = label,
                .color = color,
                .max_scale = -1,
                .min_scale = s->location_type == STATION ?
                                     20 :
                                     (s->location_type == PLATFORM ? 300 : 300),
                .radius = radius,
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
        };
        return style;
}
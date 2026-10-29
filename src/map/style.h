#ifndef MAP_STYLE_H
#define MAP_STYLE_H

#include <stddef.h>
#include <stdint.h>
#include <notcurses/notcurses.h>

#include "data/gtfs.h"

#define STOP_EXIT_COLOR 0x7A7A7A
#define STOP_STATION_COLOR 0x66CCFF
#define STOP_PLATFORM_COLOR 0xF2F2F2

struct shape_style {
        uint32_t color;
        double radius;
        int min_scale;
        int max_scale;
        bool label;
        double z_index;
};

struct shape_style style_stop(const struct stop *s);
struct shape_style style_route(const struct route *r);

#endif
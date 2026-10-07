#ifndef STOPS_H
#define STOPS_H

#include "utils/csv.h"
#include "utils/hashmap.h"
#include <stdlib.h>

enum Mode {
        BUS,
        METRO,
        TRAMWAY,
        CABLE,
        TRANSILIEN,
        RER,
        TER,
        ORLYVAL,
        FUNICULAR,
        UNKNOWN
};

struct dataset_line {
        char *route_id;
        char *route_long_name;
        char *operator_name;
        char *short_name;
        enum Mode mode;

        struct trip **trips;
        size_t trip_count;

        struct dataset_stop **stops;
        size_t stop_count;
};

struct dataset_stop {
        char *stop_id;
        char *stop_name;
        float stop_lon;
        float stop_lat;
        char *pointgeo;
        char *nom_commune;
        char *code_insee;

        char *route_id;
        char *route_long_name;

        struct dataset_line **lines;
        size_t line_count;
};

struct dataset_stop **get_dataset_stops_from_csv(csv_file *csv,
                                                 size_t *n_stops);
struct dataset_line **get_lines_from_csv(csv_file *csv, size_t *n_lines);

struct hash_map *dataset_stops_to_map(struct dataset_stop **stops,
                                      size_t *n_stops);

void free_dataset_stops(struct dataset_stop **stops, size_t n_stops);
void free_lines(struct dataset_line **lines, size_t n_lines);

#endif

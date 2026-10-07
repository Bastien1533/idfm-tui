#include "data/dataset.h"
#include "utils/hashmap.h"
#include "utils/utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct dataset_stop **get_dataset_stops_from_csv(csv_file *csv, size_t *n_stops)
{
        if (csv == NULL || csv->headers == NULL || csv->rows == NULL) {
                return NULL;
        }

        int i_stop_id = csv_col_index(csv, "stop_id");
        int i_stop_name = csv_col_index(csv, "stop_name");
        int i_stop_lon = csv_col_index(csv, "stop_lon");
        int i_stop_lat = csv_col_index(csv, "stop_lat");
        int i_pointgeo = csv_col_index(csv, "Pointgeo");
        int i_route_long_name = csv_col_index(csv, "route_long_name");
        int i_nom_commune = csv_col_index(csv, "Nom_commune");
        int i_code_insee = csv_col_index(csv, "Code_insee");

        struct dataset_stop **stops =
                malloc(csv->row_count * sizeof(struct dataset_stop *));

        for (size_t r = 0; r < csv->row_count; r++) {
                struct dataset_stop *s = calloc(1, sizeof(struct dataset_stop));
                char **row = csv->rows[r];
                if (i_stop_id >= 0)
                        s->stop_id = strdup(row[i_stop_id]);
                if (i_route_long_name >= 0)
                        s->route_long_name = strdup(row[i_route_long_name]);
                if (i_stop_name >= 0)
                        s->stop_name = strdup(row[i_stop_name]);
                if (i_stop_lon >= 0)
                        s->stop_lon = atof(
                                row[i_stop_lon]); // Converts string "2.3522" to double
                if (i_stop_lat >= 0)
                        s->stop_lat = atof(
                                row[i_stop_lat]); // Converts string "48.8566" to double
                if (i_pointgeo >= 0)
                        s->pointgeo = strdup(row[i_pointgeo]);
                if (i_nom_commune >= 0)
                        s->nom_commune = strdup(row[i_nom_commune]);
                if (i_code_insee >= 0)
                        s->code_insee = strdup(row[i_code_insee]);
                stops[r] = s;
        }
        if (n_stops != NULL) {
                *n_stops = csv->row_count;
        }
        return stops;
}

struct hash_map *dataset_stops_to_map(struct dataset_stop **stops,
                                      size_t *n_stops)
{
        if (stops == NULL) {
                return NULL;
        }
        struct hash_map *map = init_hashmap(*n_stops);
        for (size_t i = 0; i < *n_stops; i++) {
                struct dataset_stop *s = stops[i];
                if (s == NULL) {
                        continue;
                }
                hash_map_insert(map, s->stop_id, s);
        }
        return map;
}

void free_dataset_stops(struct dataset_stop **stops, size_t n_stops)
{
        if (stops == NULL) {
                return;
        }
        for (size_t i = 0; i < n_stops; i++) {
                if (stops[i] != NULL) {
                        free(stops[1]->stop_id);
                        free(stops[i]->stop_name);
                        free(stops[i]->pointgeo);
                        free(stops[i]->nom_commune);
                        free(stops[i]->code_insee);
                }
                free(stops[i]);
        }
        free(stops);
}

struct dataset_line **get_lines_from_csv(csv_file *csv, size_t *n_lines)
{
        if (csv == NULL || csv->headers == NULL || csv->rows == NULL) {
                return NULL;
        }

        int i_route_id = csv_col_index(csv, "route_id");
        int i_route_long_name = csv_col_index(csv, "route_long_name");
        int i_operator_name = csv_col_index(csv, "operator_name");
        int i_short_name = csv_col_index(csv, "short_name");

        const size_t init_line_number = 1500; // Estimated

        size_t capacity = init_line_number;
        struct dataset_line **lines =
                malloc(capacity * sizeof(struct dataset_line *));

        size_t line_count = 0;
        for (size_t r = 0; r < csv->row_count; r++) {
                if (line_count == capacity) {
                        capacity *= 2;
                        lines = realloc(lines,
                                        capacity *
                                                sizeof(struct dataset_line *));
                }

                struct dataset_line *l = NULL;
                char **row = csv->rows[r];
                if (i_route_id >= 0) {
                        if (line_in_array(lines, row[i_route_id], line_count)) {
                                continue;
                        } else {
                                l = malloc(sizeof(struct dataset_line));
                                l->route_id = strdup(row[i_route_id]);
                        }
                }
                if (l == NULL)
                        continue;
                if (i_route_long_name >= 0)
                        l->route_long_name = strdup(row[i_route_long_name]);
                if (i_operator_name >= 0)
                        l->operator_name = strdup(row[i_operator_name]);
                if (i_short_name >= 0)
                        l->short_name = strdup(row[i_short_name]);
                lines[line_count] = l;
                line_count++;
        }
        *n_lines = line_count;
        return lines;
}

/*struct trip** get_trips_from_csv(csv_file* csv, size_t* n_trips){
    if (csv == NULL || csv->headers == NULL || csv->rows == NULL) {
        return NULL;
    }

    struct trip** trips = malloc(csv->row_count * sizeof(struct trip*));
    size_t trip_count = 0;
    for (size_t r = 0; r < csv->row_count; r++) {
        struct trip* t = calloc(1, sizeof(struct trip));
        char** row = csv->rows[r];
        for (size_t c = 0; c < csv->col_count; c++) {
            char* col_name = csv->headers[c];
            if (strcmp(col_name, "route_id") == 0) {
                t->route_id = strdup(row[c]);
            }
            else if (strcmp(col_name, "service_id") == 0) {
                s->service_id = strdup(row[c]);
            }
            else if (strcmp(col_name, "trip_id") == 0) {
                s->trip_id = atof(row[c]); // Converts string "2.3522" to double
            }
            else if (strcmp(col_name, "trip_headsign") == 0) {
                s->trip_headsign = atof(row[c]); // Converts string "48.8566" to
double
            }
            else if (strcmp(col_name, "trip_short_name") == 0) {
                s->trip_short_name = strdup(row[c]);
            }
            else if (strcmp(col_name, "direction_id") == 0) {
                s->direction_id = strdup(row[c]);
            }
        }
        trips[r] = s;
    }
    *n_trips = trip_count;
    return trips;
}*/

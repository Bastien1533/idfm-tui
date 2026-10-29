#include "data/gtfs.h"
#include "data/rules.h"
#include "utils/csv.h"
#include "utils/hashmap.h"
#include "utils/utils.h"
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct agency **get_agencies_from_csv(csv_file *csv, size_t *n_agencies)
{
        if (csv == NULL || csv->headers == NULL || csv->rows == NULL) {
                return NULL;
        }

        int i_agency_id = csv_col_index(csv, "agency_id");
        int i_agency_name = csv_col_index(csv, "agency_name");
        int i_agency_url = csv_col_index(csv, "agency_url");
        int i_agency_timezone = csv_col_index(csv, "agency_timezone");
        int i_agency_lang = csv_col_index(csv, "agency_lang");
        int i_agency_phone = csv_col_index(csv, "agency_phone");
        int i_agency_email = csv_col_index(csv, "agency_email");
        int i_agency_fare_url = csv_col_index(csv, "agency_fare_url");
        int i_ticketing_deep_link_id =
                csv_col_index(csv, "ticketing_deep_link_id");

        struct agency **agencies =
                malloc(csv->row_count * sizeof(struct agency *));
        size_t agency_count = 0;
        for (size_t r = 0; r < csv->row_count; r++) {
                struct agency *a = calloc(1, sizeof(struct agency));
                char **row = csv->rows[r];
                if (i_agency_id >= 0)
                        a->agency_id = strdup(row[i_agency_id]);
                if (i_agency_name >= 0)
                        a->agency_name = strdup(row[i_agency_name]);
                if (i_agency_url >= 0)
                        a->agency_url = strdup(row[i_agency_url]);
                if (i_agency_timezone >= 0)
                        a->agency_timezone = strdup(row[i_agency_timezone]);
                if (i_agency_lang >= 0)
                        a->agency_lang = strdup(row[i_agency_lang]);
                if (i_agency_phone >= 0)
                        a->agency_phone = strdup(row[i_agency_phone]);
                if (i_agency_email >= 0)
                        a->agency_email = strdup(row[i_agency_email]);
                if (i_agency_fare_url >= 0)
                        a->agency_fare_url = strdup(row[i_agency_fare_url]);
                if (i_ticketing_deep_link_id >= 0)
                        a->ticketing_deep_link_id =
                                strdup(row[i_ticketing_deep_link_id]);
                agency_count++;
                agencies[r] = a;
        }
        if (n_agencies != NULL) {
                *n_agencies = agency_count;
        }
        return agencies;
}

struct hash_map *agencies_to_map(struct agency **agencies, size_t *n_agencies)
{
        if (agencies == NULL) {
                return NULL;
        }
        struct hash_map *map = init_hashmap(*n_agencies);
        for (size_t i = 0; i < *n_agencies; i++) {
                struct agency *a = agencies[i];
                if (a == NULL) {
                        continue;
                }
                hash_map_insert(map, a->agency_id, a);
        }
        return map;
}

struct route **get_routes_from_csv(csv_file *csv, size_t *n_routes,
                                   struct hash_map *agency_map)
{
        if (csv == NULL || csv->headers == NULL || csv->rows == NULL) {
                return NULL;
        }

        int i_route_id = csv_col_index(csv, "route_id");
        int i_agency_id = csv_col_index(csv, "agency_id");
        int i_route_short_name = csv_col_index(csv, "route_short_name");
        int i_route_long_name = csv_col_index(csv, "route_long_name");
        int i_route_desc = csv_col_index(csv, "route_desc");
        int i_route_type = csv_col_index(csv, "route_type");
        int i_route_url = csv_col_index(csv, "route_url");
        int i_route_color = csv_col_index(csv, "route_color");
        int i_route_text_color = csv_col_index(csv, "route_text_color");

        struct route **routes = malloc(csv->row_count * sizeof(struct route *));
        size_t route_count = 0;
        for (size_t r = 0; r < csv->row_count; r++) {
                struct route *ro = calloc(1, sizeof(struct route));
                char **row = csv->rows[r];
                if (i_route_id >= 0)
                        ro->route_id = strdup(row[i_route_id]);
                if (i_agency_id >= 0)
                        ro->agency = hash_map_get(agency_map, row[i_agency_id]);
                if (i_route_short_name >= 0)
                        ro->route_short_name = strdup(row[i_route_short_name]);
                if (i_route_desc >= 0)
                        ro->route_desc = strdup(row[i_route_desc]);
                if (i_route_type >= 0)
                        ro->route_type = atoi(row[i_route_type]);
                if (i_route_url >= 0)
                        ro->route_url = strdup(row[i_route_url]);
                if (i_route_color >= 0)
                        ro->route_color = strdup(row[i_route_color]);
                if (i_route_text_color >= 0)
                        ro->route_text_color = strdup(row[i_route_text_color]);
                route_count++;
                routes[r] = ro;
        }
        if (n_routes != NULL) {
                *n_routes = route_count;
        }
        return routes;
}

struct hash_map *routes_to_map(struct route **routes, size_t *n_routes)
{
        if (routes == NULL) {
                return NULL;
        }
        struct hash_map *map = init_hashmap(*n_routes);
        for (size_t i = 0; i < *n_routes; i++) {
                struct route *a = routes[i];
                if (a == NULL) {
                        continue;
                }
                hash_map_insert(map, a->route_id, a);
        }
        return map;
}

struct trip **get_trips_from_csv(csv_file *csv, size_t *n_trips,
                                 struct hash_map *route_map)
{
        if (csv == NULL || csv->headers == NULL || csv->rows == NULL) {
                return NULL;
        }

        int i_route_id = csv_col_index(csv, "route_id");
        int i_service_id = csv_col_index(csv, "service_id");
        int i_trip_id = csv_col_index(csv, "trip_id");
        int i_trip_headsign = csv_col_index(csv, "trip_headsign");
        int i_trip_short_name = csv_col_index(csv, "trip_short_name");
        int i_direction_id = csv_col_index(csv, "direction_id");
        int i_shape_id = csv_col_index(csv, "shape_id");

        struct trip **trips = malloc(csv->row_count * sizeof(struct trip *));
        size_t trip_count = 0;
        for (size_t r = 0; r < csv->row_count; r++) {
                struct trip *t = calloc(1, sizeof(struct trip));
                char **row = csv->rows[r];
                if (i_route_id >= 0)
                        t->route = hash_map_get(route_map, row[i_route_id]);
                if (i_service_id >= 0)
                        t->service_id = strdup(row[i_service_id]);
                if (i_trip_id >= 0)
                        t->trip_id = strdup(
                                row[i_trip_id]); // Converts string "2.3522" to double
                if (i_trip_headsign >= 0)
                        t->trip_headsign = strdup(row[i_trip_headsign]);
                if (i_trip_short_name >= 0)
                        t->trip_short_name = strdup(row[i_trip_short_name]);
                if (i_direction_id >= 0)
                        t->direction_id = atoi(row[i_direction_id]);
                if (i_shape_id >= 0)
                        t->shape_id = strdup(row[i_shape_id]);
                trip_count++;
                trips[r] = t;
        }
        *n_trips = trip_count;
        return trips;
}

struct hash_map *trips_to_map(struct trip **trips, size_t *n_trips,
                              size_t capacity)
{
        if (trips == NULL) {
                return NULL;
        }
        struct hash_map *map = init_hashmap(capacity);
        for (size_t i = 0; i < *n_trips; i++) {
                struct trip *t = trips[i];
                if (t == NULL) {
                        continue;
                }
                hash_map_insert(map, t->trip_id, t);
        }
        return map;
}

struct shape **get_shapes_from_csv(csv_file *csv, size_t *n_shapes,
                                   double *lat_ref)
{
        if (csv == NULL || csv->headers == NULL || csv->rows == NULL) {
                return NULL;
        }

        double min_lat = INT_MAX;
        double max_lat = INT_MIN;

        int i_shape_id = csv_col_index(csv, "shape_id");
        int i_shape_pt_lat = csv_col_index(csv, "shape_pt_lat");
        int i_shape_pt_lon = csv_col_index(csv, "shape_pt_lon");
        int i_shape_pt_sequence = csv_col_index(csv, "shape_pt_sequence");

        struct hash_map *shape_map = init_hashmap(50);

        struct shape **shapes = malloc(csv->row_count * sizeof(struct shape *));
        size_t shape_count = 0;
        for (size_t r = 0; r < csv->row_count; r++) {
                char **row = csv->rows[r];

                char *shape_id = strdup(row[i_shape_id]);
                struct shape *s = hash_map_get(shape_map, shape_id);
                if (s == NULL) {
                        s = calloc(1, sizeof(struct shape));
                        s->shape_id = shape_id;
                        s->points_n = 0;
                        hash_map_insert(shape_map, shape_id, s);
                        shapes[shape_count] = s;
                        shape_count++;
                }

                struct shape_seq **points_t =
                        realloc(s->points,
                                (s->points_n + 1) * sizeof(struct shape_seq *));
                struct shape_seq *seq = calloc(1, sizeof(struct shape_seq));
                seq->shape_pt_lat = atof(row[i_shape_pt_lat]);
                seq->shape_pt_lon = atof(row[i_shape_pt_lon]);
                seq->shape_pt_sequence = atoi(row[i_shape_pt_sequence]);

                min_lat = fmin(min_lat, seq->shape_pt_lat);
                max_lat = fmax(max_lat, seq->shape_pt_lat);

                s->points = points_t;
                s->points[s->points_n++] = seq;
        }
        if (min_lat == INT_MAX || max_lat == INT_MIN)
                *lat_ref = 48.85;
        else
                *lat_ref = (max_lat + min_lat) / 2;
        free_hashmap(shape_map);
        *n_shapes = shape_count;
        return shapes;
}

void associate_shape_route(struct trip **trips, size_t n_trips,
                           struct hash_map *shape_map)
{
        if (trips == NULL || shape_map == NULL)
                return;
        for (size_t i = 0; i < n_trips; i++) {
                struct trip *t = trips[i];
                if (t->shape_id == NULL)
                        continue;
                struct shape *s = hash_map_get(shape_map, trips[i]->shape_id);
                if (s == NULL)
                        return;
                s->route = t->route;
        }
}

struct hash_map *shapes_to_map(struct shape **shapes, size_t *n_shapes)
{
        if (shapes == NULL) {
                return NULL;
        }
        struct hash_map *map = init_hashmap(*n_shapes);
        for (size_t i = 0; i < *n_shapes; i++) {
                struct shape *s = shapes[i];
                if (s == NULL) {
                        continue;
                }
                hash_map_insert(map, s->shape_id, s);
        }
        return map;
}

struct attribution **get_attributions_from_csv(csv_file *csv,
                                               size_t *n_attributions,
                                               struct hash_map *routes)
{
        if (csv == NULL || csv->headers == NULL || csv->rows == NULL) {
                return NULL;
        }

        int i_attribution_id = csv_col_index(csv, "attribution_id");
        int i_route_id = csv_col_index(csv, "route_id");
        int i_trip_id = csv_col_index(csv, "trip_id");
        int i_is_operator = csv_col_index(csv, "is_operator");
        int i_organization_name = csv_col_index(csv, "organization_name");
        int i_attribution_url = csv_col_index(csv, "attribution_url");
        int i_attribution_email = csv_col_index(csv, "attribution_email");
        int i_attribution_phone = csv_col_index(csv, "attribution_phone");

        struct attribution **attributions =
                malloc(csv->row_count * sizeof(struct attribution *));
        size_t attribution_count = 0;
        for (size_t r = 0; r < csv->row_count; r++) {
                struct attribution *a = calloc(1, sizeof(struct attribution));
                char **row = csv->rows[r];
                if (i_attribution_id >= 0)
                        a->attribution_id = strdup(row[i_attribution_id]);
                if (i_route_id >= 0)
                        a->route = hash_map_get(routes, row[i_route_id]);
                if (i_trip_id >= 0)
                        a->trip_id = strdup(row[i_trip_id]);
                if (i_is_operator >= 0)
                        a->is_operator = atoi(row[i_is_operator]);
                if (i_organization_name >= 0)
                        a->organization_name = strdup(row[i_organization_name]);
                if (i_attribution_url >= 0)
                        a->attribution_url = strdup(row[i_attribution_url]);
                if (i_attribution_email >= 0)
                        a->attribution_email = strdup(row[i_attribution_email]);
                if (i_attribution_phone >= 0)
                        a->attribution_phone = strdup(row[i_attribution_phone]);
                attribution_count++;
                attributions[r] = a;
        }
        if (n_attributions != NULL) {
                *n_attributions = attribution_count;
        }
        return attributions;
}

struct hash_map *attributions_to_map(struct attribution **attributions,
                                     size_t *n_attribution)
{
        if (attributions == NULL) {
                return NULL;
        }
        struct hash_map *map = init_hashmap(*n_attribution);
        for (size_t i = 0; i < *n_attribution; i++) {
                struct attribution *a = attributions[i];
                if (a == NULL) {
                        continue;
                }
                hash_map_insert(map, a->route->route_id, a);
        }
        return map;
}

struct stop **get_stops_from_csv(csv_file *csv, size_t *n_stops)
{
        if (csv == NULL || csv->headers == NULL || csv->rows == NULL) {
                return NULL;
        }

        int i_stop_id = csv_col_index(csv, "stop_id");
        int i_stop_code = csv_col_index(csv, "stop_code");
        int i_stop_name = csv_col_index(csv, "stop_name");
        int i_stop_desc = csv_col_index(csv, "stop_desc");
        int i_stop_lon = csv_col_index(csv, "stop_lon");
        int i_stop_lat = csv_col_index(csv, "stop_lat");
        int i_zone_id = csv_col_index(csv, "zone_id");
        int i_stop_url = csv_col_index(csv, "stop_url");
        int i_location_type = csv_col_index(csv, "location_type");
        int i_parent_station = csv_col_index(csv, "parent_station");
        int i_stop_timezone = csv_col_index(csv, "stop_timezone");
        int i_level_id = csv_col_index(csv, "level_id");
        int i_wheelchair_boarding = csv_col_index(csv, "wheelchair_boarding");
        int i_platform_code = csv_col_index(csv, "platform_code");

        struct stop **stops = malloc(csv->row_count * sizeof(struct stop *));
        size_t stop_count = 0;
        for (size_t r = 0; r < csv->row_count; r++) {
                struct stop *s = calloc(1, sizeof(struct stop));
                char **row = csv->rows[r];
                if (i_stop_id >= 0)
                        s->stop_id = strdup(row[i_stop_id]);
                if (i_stop_code >= 0)
                        s->stop_code = atoi(row[i_stop_code]);
                if (i_stop_name >= 0)
                        s->stop_name = strdup(row[i_stop_name]);
                if (i_stop_desc >= 0)
                        s->stop_desc = strdup(row[i_stop_desc]);
                if (i_stop_lon >= 0)
                        s->stop_lon = atof(row[i_stop_lon]);
                if (i_stop_lat >= 0)
                        s->stop_lat = atof(row[i_stop_lat]);
                if (i_zone_id >= 0)
                        s->zone_id = atoi(row[i_zone_id]);
                if (i_stop_url >= 0)
                        s->stop_url = strdup(row[i_stop_url]);
                if (i_location_type >= 0)
                        s->location_type = atoi(row[i_location_type]);
                if (i_parent_station >= 0)
                        s->parent_station_id = strdup(row[i_parent_station]);
                if (i_stop_timezone >= 0)
                        s->stop_timezone = strdup(row[i_stop_timezone]);
                if (i_level_id >= 0)
                        s->level_id = strdup(row[i_level_id]);
                if (i_wheelchair_boarding >= 0)
                        s->wheelchair_boarding =
                                atoi(row[i_wheelchair_boarding]);
                if (i_platform_code >= 0)
                        s->platform_code = strdup(row[i_platform_code]);

                s->assoc_routes = NULL;
                s->n_assoc_routes = 0;
                s->stop_role = STOP_ROLE_DEFAULT;

                stop_count++;
                s->idx = r;
                s->parent_station = NULL;
                stops[r] = s;
        }
        if (n_stops != NULL) {
                *n_stops = stop_count;
        }
        return stops;
}

struct hash_map *stops_to_map(struct stop **stops, size_t *n_stops)
{
        if (stops == NULL) {
                return NULL;
        }
        struct hash_map *map = init_hashmap(*n_stops);
        for (size_t i = 0; i < *n_stops; i++) {
                struct stop *s = stops[i];
                if (s == NULL) {
                        continue;
                }
                hash_map_insert(map, s->stop_id, s);
        }
        return map;
}

void link_parent_stops(struct stop **stops, size_t n_stops,
                       struct hash_map *stop_map, struct transfer ***transfers,
                       size_t *n_transfers, struct hash_map *transfer_map)
{
        if (stops == NULL || stop_map == NULL || stop_map->nodes == NULL) {
                return;
        }
        for (size_t i = 0; i < n_stops; i++) {
                struct stop *s = stops[i];
                if (s == NULL || s->parent_station_id == NULL ||
                    s->parent_station != NULL)
                        continue;
                struct stop *p = hash_map_get(stop_map, s->parent_station_id);
                if (p == NULL)
                        continue;

                s->parent_station = p;
                char *composite_id = NULL;
                asprintf(&composite_id, "%s_%s", p->stop_id, s->stop_id);
                if (hash_map_get(transfer_map, composite_id) == NULL) {
                        struct transfer **new_transfers = realloc(
                                *transfers,
                                (*n_transfers + 2) * sizeof(struct transfer *));
                        if (new_transfers != NULL) {
                                *transfers = new_transfers;

                                struct transfer *t1 =
                                        calloc(1, sizeof(struct transfer));
                                t1->from_stop = p;
                                t1->to_stop = s;
                                t1->min_transfer_time = 0;
                                t1->transfer_type = 2;

                                struct transfer *t2 =
                                        calloc(1, sizeof(struct transfer));
                                t2->from_stop = s;
                                t2->to_stop = p;
                                t2->min_transfer_time = 0;
                                t2->transfer_type = 2;

                                (*transfers)[*n_transfers] = t1;
                                (*n_transfers)++;
                                (*transfers)[*n_transfers] = t2;
                                (*n_transfers)++;

                                char *rev_composite_id = NULL;
                                asprintf(&rev_composite_id, "%s_%s", s->stop_id,
                                         p->stop_id);

                                hash_map_insert(transfer_map, composite_id, t1);
                                hash_map_insert(transfer_map, rev_composite_id,
                                                t2);

                                p->transfers = realloc(
                                        p->transfers,
                                        (p->transfer_count + 1) *
                                                sizeof(struct transfer *));
                                p->transfers[p->transfer_count++] = t1;

                                s->transfers = realloc(
                                        s->transfers,
                                        (s->transfer_count + 1) *
                                                sizeof(struct transfer *));
                                s->transfers[s->transfer_count++] = t2;
                        }
                } else {
                        free(composite_id);
                }

                if (p->child_stops == NULL) {
                        p->child_stops = calloc(1, sizeof(struct stop *));
                        p->child_stops[0] = s;
                        p->n_children = 1;
                } else {
                        p->child_stops = realloc(p->child_stops,
                                                 (p->n_children + 1) *
                                                         sizeof(struct stop *));
                        p->child_stops[p->n_children] = s;
                        p->n_children++;
                }
        }
}

int cmp_routes(const void *x, const void *y)
{
        const struct route *a = *(const struct route *const *)x;
        const struct route *b = *(const struct route *const *)y;

        if (a->route_type != b->route_type)
                return a->route_type < b->route_type ? -1 : 1;
        return strcasecmp(a->route_short_name, b->route_short_name);
}

void populate_stop_routes(struct stop_time **stop_times, size_t stop_times_n,
                          struct stop **stops, size_t stops_n)
{
        if (stop_times == NULL || stop_times_n == 0 || stops == NULL ||
            stops_n == 0)
                return;
        for (size_t i = 0; i < stop_times_n; i++) {
                struct stop_time *st = stop_times[i];
                if (st == NULL || st->trip == NULL || st->trip->route == NULL ||
                    st->stop == NULL)
                        continue;

                struct stop *st_stop = st->stop->parent_station == NULL ?
                                               st->stop :
                                               st->stop->parent_station;
                struct route **assoc_routes_t = realloc(
                        st_stop->assoc_routes,
                        (st_stop->n_assoc_routes + 1) * sizeof(struct route *));
                if (assoc_routes_t == NULL)
                        continue;
                st_stop->assoc_routes = assoc_routes_t;
                st_stop->assoc_routes[st_stop->n_assoc_routes++] =
                        st->trip->route;
        }

        for (size_t i = 0; i < stops_n; i++) {
                struct stop *st = stops[i];
                if (st == NULL || st->assoc_routes == NULL ||
                    st->n_assoc_routes == 0)
                        continue;
                qsort(st->assoc_routes, st->n_assoc_routes,
                      sizeof(struct route *), cmp_routes); //Sort

                // Unique deduplicating
                size_t w = 0;
                for (size_t r = 0; r < st->n_assoc_routes; r++) {
                        if (w == 0 ||
                            st->assoc_routes[r] != st->assoc_routes[w - 1])
                                st->assoc_routes[w++] = st->assoc_routes[r];
                }
                st->n_assoc_routes = w;
                st->assoc_routes =
                        realloc(st->assoc_routes,
                                st->n_assoc_routes * sizeof(struct route *));
        }
}

int cmp_freq(void *thunk, const void *a, const void *b)
{
        struct hash_map *freq = thunk;

        const struct stop *sa = *(struct stop *const *)a;
        const struct stop *sb = *(struct stop *const *)b;

        intptr_t ca = (intptr_t)hash_map_get(freq, sa->stop_id);
        intptr_t cb = (intptr_t)hash_map_get(freq, sb->stop_id);

        return (cb > ca) - (cb < ca);
}

void populate_route_ends(struct trip **trips, size_t n_trips,
                         struct hash_map *stop_map, struct route **routes,
                         size_t n_routes)
{
        if (trips == NULL || routes == NULL || stop_map == NULL)
                return;
        for (size_t i = 0; i < n_trips; i++) {
                struct trip *t = trips[i];
                if (t->route == NULL || t->stop_times == NULL)
                        continue;
                struct route *r = t->route;
                struct stop *end = NULL;
                if (t->stop_time_count > 0)
                        end = t->stop_times[t->stop_time_count - 1]->stop;
                if (end == NULL)
                        continue;
                if (r->end_stop_freq == NULL)
                        r->end_stop_freq = init_hashmap(10);
                intptr_t cur_freq = (intptr_t)hash_map_get(
                        r->end_stop_freq,
                        end->stop_id); //We encode the value in the pointer directly to avoid allocating
                cur_freq++;
                hash_map_insert(r->end_stop_freq, end->stop_id,
                                (void *)cur_freq);
        }

        for (size_t i = 0; i < n_routes; i++) {
                struct route *r = routes[i];
                if (r->end_stop_freq == NULL)
                        continue;
                int total_trips = 0;
                int initial_n_ends = 0;
                int inserted_ends = 0;
                for (size_t b = 0; b < r->end_stop_freq->capacity; b++) {
                        for (struct hash_node *n = r->end_stop_freq->nodes[b];
                             n; n = n->next) {
                                intptr_t val = (intptr_t)n->value;
                                total_trips += val;
                                initial_n_ends++;
                        }
                }
                struct stop **end_stops =
                        calloc(initial_n_ends, sizeof(struct stop *));
                if (end_stops == NULL)
                        continue;

                for (size_t b = 0; b < r->end_stop_freq->capacity; b++) {
                        for (struct hash_node *n = r->end_stop_freq->nodes[b];
                             n; n = n->next) {
                                intptr_t val = (intptr_t)n->value;
                                double thr = (0.005 * total_trips) < 20 ?
                                                     20 :
                                                     (0.005 * total_trips);
                                if (val >= thr) {
                                        struct stop *s =
                                                hash_map_get(stop_map, n->key);
                                        if (s == NULL)
                                                continue;
                                        end_stops[inserted_ends++] = s;
                                }
                        }
                }
                r->end_stops = realloc(end_stops,
                                       inserted_ends * sizeof(struct stop *));
                r->n_end_stops = inserted_ends;

                qsort_r(r->end_stops, r->n_end_stops, sizeof(struct stop *),
                        r->end_stop_freq, cmp_freq);
        }
}

struct transfer **get_transfers_from_csv(csv_file *csv, size_t *n_transfers,
                                         struct hash_map *stops)
{
        if (csv == NULL || csv->headers == NULL || csv->rows == NULL) {
                return NULL;
        }

        int i_from_stop_id = csv_col_index(csv, "from_stop_id");
        int i_to_stop_id = csv_col_index(csv, "to_stop_id");
        int i_transfer_type = csv_col_index(csv, "transfer_type");
        int i_min_transfer_time = csv_col_index(csv, "min_transfer_time");

        struct transfer **transfers =
                malloc(csv->row_count * sizeof(struct transfer *));
        size_t transfer_count = 0;
        for (size_t r = 0; r < csv->row_count; r++) {
                struct transfer *t = calloc(1, sizeof(struct transfer));
                char **row = csv->rows[r];
                if (i_from_stop_id >= 0)
                        t->from_stop = hash_map_get(stops, row[i_from_stop_id]);
                if (i_to_stop_id >= 0)
                        t->to_stop = hash_map_get(stops, row[i_to_stop_id]);
                if (i_transfer_type >= 0)
                        t->transfer_type = atoi(row[i_transfer_type]);
                if (i_min_transfer_time >= 0)
                        t->min_transfer_time = atoi(row[i_min_transfer_time]);
                if (t->from_stop != NULL && t->to_stop != NULL) {
                        if (strcmp(t->from_stop->parent_station_id,
                                   t->to_stop->parent_station_id) == 0) {
                                t->is_inter_parent_transfer = true;
                        } else {
                                t->is_inter_parent_transfer = false;
                        }
                }
                if (t->from_stop != NULL) {
                        if (t->from_stop->transfers == NULL) { // INIT TRANSFERS
                                t->from_stop->transfers =
                                        calloc(1, sizeof(struct transfer *));
                                t->from_stop->transfers[0] = t;
                                t->from_stop->transfer_count = 1;
                        } else {
                                t->from_stop->transfers = realloc(
                                        t->from_stop->transfers,
                                        (t->from_stop->transfer_count + 1) *
                                                sizeof(struct transfer *));
                                t->from_stop->transfers
                                        [t->from_stop->transfer_count] = t;
                                t->from_stop->transfer_count++;
                        }
                }
                transfer_count++;
                transfers[r] = t;
        }
        if (n_transfers != NULL) {
                *n_transfers = transfer_count;
        }
        return transfers;
}

struct pathway **get_pathways_from_csv(csv_file *csv, size_t *n_pathways,
                                       struct hash_map *stops)
{
        if (csv == NULL || csv->headers == NULL || csv->rows == NULL) {
                return NULL;
        }

        int i_pathway_id = csv_col_index(csv, "pathway_id");
        int i_from_stop_id = csv_col_index(csv, "from_stop_id");
        int i_to_stop_id = csv_col_index(csv, "to_stop_id");
        int i_pathway_mode = csv_col_index(csv, "pathway_mode");
        int i_is_bidirectional = csv_col_index(csv, "is_bidirectional");
        int i_length = csv_col_index(csv, "length");
        int i_traversal_time = csv_col_index(csv, "traversal_time");
        int i_stair_count = csv_col_index(csv, "stair_count");
        int i_max_slope = csv_col_index(csv, "max_slope");
        int i_min_width = csv_col_index(csv, "min_width");
        int i_signposted_as = csv_col_index(csv, "signposted_as");
        int i_reversed_signposted_as =
                csv_col_index(csv, "reversed_signposted_as");

        struct pathway **pathways =
                malloc(csv->row_count * sizeof(struct pathway *));
        size_t pathway_count = 0;
        for (size_t r = 0; r < csv->row_count; r++) {
                struct pathway *p = calloc(1, sizeof(struct pathway));
                char **row = csv->rows[r];
                if (i_from_stop_id >= 0)
                        p->from_stop = hash_map_get(stops, row[i_from_stop_id]);
                if (i_to_stop_id >= 0)
                        p->to_stop = hash_map_get(stops, row[i_to_stop_id]);
                if (i_pathway_id >= 0)
                        p->pathway_id = strdup(row[i_pathway_id]);
                if (i_pathway_mode >= 0)
                        p->pathway_mode = atoi(row[i_pathway_mode]);
                if (i_is_bidirectional >= 0)
                        p->is_bidirectional = atoi(row[i_is_bidirectional]);
                if (i_length >= 0)
                        p->length = atof(row[i_length]);
                if (i_traversal_time >= 0)
                        p->traversal_time = atoi(row[i_traversal_time]);
                if (i_max_slope >= 0)
                        p->max_slope = atoi(row[i_max_slope]);
                if (i_min_width >= 0)
                        p->min_width = atoi(row[i_min_width]);
                if (i_signposted_as >= 0)
                        p->signposted_as = strdup(row[i_signposted_as]);
                if (i_reversed_signposted_as >= 0)
                        p->reversed_signposted_as =
                                strdup(row[i_reversed_signposted_as]);

                pathway_count++;
                pathways[r] = p;
        }
        if (n_pathways != NULL) {
                *n_pathways = pathway_count;
        }
        return pathways;
}

struct hash_map *pathways_to_map(struct pathway **pathways, size_t *n_pathways)
{
        if (pathways == NULL) {
                return NULL;
        }
        struct hash_map *map = init_hashmap(*n_pathways);
        for (size_t i = 0; i < *n_pathways; i++) {
                struct pathway *p = pathways[i];
                if (p == NULL) {
                        continue;
                }
                char *composite_id = NULL;
                asprintf(&composite_id, "%s_%s", p->from_stop->stop_id,
                         p->to_stop->stop_id);
                hash_map_insert(map, composite_id, p);
        }
        return map;
}

struct hash_map *transfers_to_map(struct transfer **transfers,
                                  size_t *n_transfers)
{
        if (transfers == NULL) {
                return NULL;
        }
        struct hash_map *map = init_hashmap(*n_transfers);
        for (size_t i = 0; i < *n_transfers; i++) {
                struct transfer *t = transfers[i];
                if (t == NULL) {
                        continue;
                }
                char *composite_id = NULL;
                asprintf(&composite_id, "%s_%s", t->from_stop->stop_id,
                         t->to_stop->stop_id);
                hash_map_insert(map, composite_id, t);
        }
        return map;
}

struct stop_time **get_stop_times_from_csv(csv_file *csv, size_t *n_stop_times,
                                           struct hash_map *trip_map,
                                           struct hash_map *stop_map)
{
        if (csv == NULL || csv->headers == NULL || csv->rows == NULL) {
                return NULL;
        }

        int i_trip_id = csv_col_index(csv, "trip_id");
        int i_arrival_time = csv_col_index(csv, "arrival_time");
        int i_departure_time = csv_col_index(csv, "departure_time");
        int i_start_pickup_drop_off_window =
                csv_col_index(csv, "start_pickup_drop_off_window");
        int i_end_pickup_drop_off_window =
                csv_col_index(csv, "end_pickup_drop_off_window");
        int i_stop_id = csv_col_index(csv, "stop_id");
        int i_stop_sequence = csv_col_index(csv, "stop_sequence");
        int i_pickup_type = csv_col_index(csv, "pickup_type");
        int i_drop_off_type = csv_col_index(csv, "drop_off_type");

        struct stop_time **stop_times =
                malloc(csv->row_count * sizeof(struct stop_time *));
        size_t stop_time_count = 0;
        for (size_t r = 0; r < csv->row_count; r++) {
                struct stop_time *st = calloc(1, sizeof(struct stop_time));
                char **row = csv->rows[r];
                if (i_trip_id >= 0)
                        st->trip = hash_map_get(trip_map, row[i_trip_id]);
                if (i_arrival_time >= 0)
                        st->arrival_time =
                                gtfs_time_from_midnight(row[i_arrival_time]);
                if (i_departure_time >= 0)
                        st->departure_time =
                                gtfs_time_from_midnight(row[i_departure_time]);
                if (i_start_pickup_drop_off_window >= 0)
                        st->start_pickup_drop_off_window = 0; // Not documented
                if (i_end_pickup_drop_off_window >= 0)
                        st->end_pickup_drop_off_window = 0; // Not documented
                if (i_stop_id >= 0)
                        st->stop = hash_map_get(stop_map, row[i_stop_id]);
                if (i_stop_sequence >= 0)
                        st->stop_sequence = atoi(row[i_stop_sequence]);
                if (i_pickup_type >= 0)
                        st->pickup_type = atoi(row[i_pickup_type]);
                if (i_drop_off_type >= 0)
                        st->drop_off_type = atoi(row[i_drop_off_type]);
                stop_time_count++;
                stop_times[r] = st;

                if (st->trip != NULL) {
                        st->trip->stop_times =
                                realloc(st->trip->stop_times,
                                        (st->trip->stop_time_count + 1) *
                                                sizeof(struct stop_time *));
                        st->trip->stop_times[st->trip->stop_time_count] = st;
                        st->trip->stop_time_count++;
                }
        }
        if (n_stop_times != NULL) {
                *n_stop_times = stop_time_count;
        }
        return stop_times;
}

void free_agencies(struct agency **agencies, size_t n_agencies)
{
        if (agencies == NULL) {
                return;
        }
        for (size_t i = 0; i < n_agencies; i++) {
                struct agency *a = agencies[i];
                if (a == NULL) {
                        continue;
                }
                free(a->agency_id);
                free(a->agency_name);
                free(a->agency_url);
                free(a->agency_timezone);
                free(a->agency_lang);
                free(a->agency_phone);
                free(a->agency_email);
                free(a->agency_fare_url);
                free(a->ticketing_deep_link_id);
                free(a);
        }
        free(agencies);
}

void free_stops(struct stop **stops, size_t n_stops)
{
        if (stops == NULL) {
                return;
        }
        for (size_t i = 0; i < n_stops; i++) {
                struct stop *s = stops[i];
                if (s == NULL) {
                        continue;
                }
                free(s->stop_id);
                free(s->stop_name);
                free(s->stop_desc);
                free(s->stop_url);
                free(s->parent_station);
                free(s->stop_timezone);
                free(s->level_id);
                free(s->platform_code);
                free(s->transfers);
                free(s);
        }
        free(stops);
}

void free_transfers(struct transfer **transfers, size_t n_transfers)
{
        if (transfers == NULL) {
                return;
        }
        for (size_t i = 0; i < n_transfers; i++) {
                free(transfers[i]);
        }
        free(transfers);
}

void free_pathways(struct pathway **pathways, size_t n_pathways)
{
        if (pathways == NULL) {
                return;
        }
        for (size_t i = 0; i < n_pathways; i++) {
                struct pathway *p = pathways[i];
                if (p == NULL) {
                        continue;
                }
                free(p->pathway_id);
                free(p->signposted_as);
                free(p->reversed_signposted_as);
                free(p);
        }
        free(pathways);
}

void free_attributions(struct attribution **attributions, size_t n_attributions)
{
        if (attributions == NULL) {
                return;
        }
        for (size_t i = 0; i < n_attributions; i++) {
                struct attribution *a = attributions[i];
                if (a == NULL) {
                        continue;
                }
                free(a->attribution_id);
                free(a->trip_id);
                free(a->organization_name);
                free(a->attribution_url);
                free(a->attribution_email);
                free(a->attribution_phone);
                free(a);
        }
        free(attributions);
}

void free_routes(struct route **routes, size_t n_routes)
{
        if (routes == NULL) {
                return;
        }
        for (size_t i = 0; i < n_routes; i++) {
                struct route *r = routes[i];
                if (r == NULL) {
                        continue;
                }
                free(r->route_id);
                free(r->route_short_name);
                free(r->route_long_name);
                free(r->route_desc);
                free(r->route_url);
                free(r->route_color);
                free(r->route_text_color);
                free(r->end_stops);
                free_hashmap(r->end_stop_freq);
                free(r);
        }
        free(routes);
}

void free_trips(struct trip **trips, size_t n_trips)
{
        if (trips == NULL) {
                return;
        }
        for (size_t i = 0; i < n_trips; i++) {
                struct trip *t = trips[i];
                if (t == NULL) {
                        continue;
                }
                free(t->service_id);
                free(t->trip_id);
                free(t->trip_headsign);
                free(t->trip_short_name);
                free(t->shape_id);
                free(t->stop_times);
                free(t);
        }
        free(trips);
}

void free_stop_times(struct stop_time **stop_times, size_t n_stop_times)
{
        if (stop_times == NULL) {
                return;
        }
        for (size_t i = 0; i < n_stop_times; i++) {
                free(stop_times[i]);
        }
        free(stop_times);
}

#include "algo/raptor.h"
#include "data/gtfs.h"
#include "utils/utils.h"
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_trip_sequence_key(struct trip *trip)
{
        size_t total_len = 0;
        for (size_t i = 0; i < trip->stop_time_count; i++) {
                if (trip->stop_times && trip->stop_times[i]->stop &&
                    trip->stop_times[i]->stop->stop_name) {
                        total_len +=
                                strlen(trip->stop_times[i]->stop->stop_name);
                }
        }

        char *dest = malloc((total_len + 1) * sizeof(char));

        char *curr = dest;
        for (size_t i = 0; i < trip->stop_time_count; i++) {
                if (trip->stop_times && trip->stop_times[i]->stop &&
                    trip->stop_times[i]->stop->stop_name) {
                        size_t len =
                                strlen(trip->stop_times[i]->stop->stop_name);
                        memcpy(curr, trip->stop_times[i]->stop->stop_name, len);
                        curr += len;
                }
        }
        *curr = '\0';
        return dest;
}

void sort_stop_time_blocks_bubble(struct stop_time ***stop_times,
                                  size_t n_trips)
{
        if (n_trips == 0) {
                return;
        }
        for (size_t i = n_trips - 1; i > 0; i--) {
                bool sorted = true;
                for (size_t j = 0; j < i; j++) {
                        if (stop_times[j + 1][0]->departure_time <
                            stop_times[j][0]->departure_time) {
                                struct stop_time **temp_j1 = stop_times[j + 1];
                                stop_times[j + 1] = stop_times[j];
                                stop_times[j] = temp_j1;
                                sorted = false;
                        }
                }
                if (sorted)
                        break;
        }
}

struct raptor_route **
build_raptor_routes(struct trip **trips, const size_t n_trips, size_t *n_routes)
{
        struct hash_map *map = init_hashmap(n_trips);

        for (size_t i = 0; i < n_trips; i++) {
                struct trip *t = trips[i];
                char *seq = get_trip_sequence_key(t);

                struct raptor_route *route = hash_map_get(map, seq);
                if (route == NULL) {
                        struct raptor_route *r =
                                malloc(sizeof(struct raptor_route));
                        r->route = t->route;
                        r->stop_times = calloc(1, sizeof(struct stop_time **));
                        r->stop_times[0] = calloc(t->stop_time_count,
                                                  sizeof(struct stop_time *));
                        r->n_trips = 1;
                        r->route_stops =
                                calloc(t->stop_time_count,
                                       sizeof(struct raptor_route_stop *));
                        r->n_stops = t->stop_time_count;
                        for (size_t i = 0; i < t->stop_time_count; i++) {
                                if (t->stop_times && t->stop_times[i]->stop &&
                                    t->stop_times[i]->stop->stop_name) {
                                        r->route_stops[i] = malloc(sizeof(
                                                struct raptor_route_stop));
                                        r->route_stops[i]->stop =
                                                t->stop_times[i]->stop;
                                        r->route_stops[i]->pos_in_route = i;
                                        r->route_stops[i]->route = r;

                                        r->stop_times[0][i] = t->stop_times[i];
                                }
                        }
                        hash_map_insert(map, seq, r);
                } else {
                        route->stop_times =
                                realloc(route->stop_times,
                                        (route->n_trips + 1) *
                                                sizeof(struct stop_time **));
                        route->n_trips++;
                        route->stop_times[route->n_trips - 1] = calloc(
                                t->stop_time_count, sizeof(struct stop_time *));
                        for (size_t i = 0; i < t->stop_time_count; i++) {
                                if (t->stop_times && t->stop_times[i]->stop &&
                                    t->stop_times[i]->stop->stop_name) {
                                        route->stop_times[route->n_trips - 1]
                                                         [i] = t->stop_times[i];
                                }
                        }
                }

                free(seq);
        }

        struct raptor_route **routes =
                calloc(map->size, sizeof(struct raptor_route *));

        size_t idx = 0;
        for (size_t i = 0; i < map->capacity; i++) {
                if (map->nodes[i] != NULL) {
                        struct hash_node *curr = map->nodes[i];
                        while (curr != NULL) {
                                struct hash_node *next = curr->next;
                                struct raptor_route *r = curr->value;
                                if (r) {
                                        if (r->stop_times == NULL) {
                                                printf("fuck\n");
                                        } else {
                                                sort_stop_time_blocks_bubble(
                                                        r->stop_times,
                                                        r->n_trips);
                                                routes[idx] = r;
                                                r->idx = idx;
                                                idx++;
                                        }
                                }
                                curr = next;
                        }
                }
        }
        if (n_routes != NULL) {
                *n_routes = idx;
        }
        free_hashmap(map);
        return routes;
}

struct raptor_stop **build_raptor_stops(struct stop **all_stops,
                                        const size_t n_all_stops,
                                        struct raptor_route **routes,
                                        const size_t n_routes,
                                        size_t *n_raptor_stops)
{
        struct hash_map *map = init_hashmap(n_all_stops);
        struct raptor_stop **stops =
                calloc(n_all_stops, sizeof(struct raptor_stop *));

        // 1. Initialize ALL stops upfront (including unvisited ones)
        for (size_t i = 0; i < n_all_stops; i++) {
                struct stop *s = all_stops[i];
                if (s == NULL)
                        continue;

                struct raptor_stop *rs = calloc(1, sizeof(struct raptor_stop));
                rs->stop = s;
                rs->idx = i;
                rs->n_routes = 0;
                rs->routes_stops = NULL;

                // Populate transfers ONCE per stop to fix the transfer duplication bug
                rs->n_transfers = s->transfer_count;
                if (s->transfer_count > 0) {
                        rs->transfers = calloc(s->transfer_count,
                                               sizeof(struct transfer *));
                        for (size_t x = 0; x < s->transfer_count; x++) {
                                rs->transfers[x] = s->transfers[x];
                        }
                } else {
                        rs->transfers = NULL;
                }

                stops[i] = rs;
                hash_map_insert(map, s->stop_id, rs);
        }

        // 2. Link routes to existing raptor_stops
        for (size_t i = 0; i < n_routes; i++) {
                struct raptor_route *r = routes[i];
                for (size_t j = 0; j < r->n_stops; j++) {
                        struct raptor_route_stop *s = r->route_stops[j];
                        if (s == NULL || s->stop == NULL)
                                continue;

                        struct raptor_stop *rs =
                                hash_map_get(map, s->stop->stop_id);
                        if (rs != NULL) {
                                rs->n_routes++;
                                rs->routes_stops = realloc(
                                        rs->routes_stops,
                                        rs->n_routes *
                                                sizeof(struct raptor_route_stop
                                                               *));
                                rs->routes_stops[rs->n_routes - 1] = s;
                                s->rstop = rs;
                        }
                }
        }

        for (size_t i = 0; i < n_all_stops; i++) {
                struct raptor_stop *rs = stops[i];
                if (rs == NULL)
                        continue;

                for (size_t ti = 0; ti < rs->n_transfers; ti++) {
                        if (rs->transfers[ti] != NULL &&
                            rs->transfers[ti]->to_stop != NULL) {
                                struct raptor_stop *to_rstop = hash_map_get(
                                        map,
                                        rs->transfers[ti]->to_stop->stop_id);
                                rs->transfers[ti]->to_rstop = to_rstop;
                        }
                }
        }

        if (n_raptor_stops != NULL) {
                *n_raptor_stops = n_all_stops;
        }

        free_hashmap(map);
        return stops;
}

struct hash_map *raptor_stops_to_map(struct raptor_stop **stops,
                                     size_t *n_raptor_stops)
{
        if (stops == NULL) {
                return NULL;
        }
        struct hash_map *map = init_hashmap(*n_raptor_stops);
        for (size_t i = 0; i < *n_raptor_stops; i++) {
                struct raptor_stop *s = stops[i];
                if (s == NULL) {
                        continue;
                }
                hash_map_insert(map, s->stop->stop_id, s);
        }
        return map;
}

bool raptor_itineray_add_leg(struct raptor_itinerary *itinerary,
                             struct transfer *transfer, struct trip *trip,
                             struct raptor_stop *dep, struct raptor_stop *arr,
                             size_t dep_time, size_t arr_time)
{
        if (itinerary == NULL || dep == NULL || arr == NULL ||
            (trip == NULL && transfer == NULL)) {
                return false;
        }

        struct raptor_leg *leg = calloc(1, sizeof(struct raptor_leg));
        if (leg == NULL) {
                return false;
        }

        struct raptor_leg **new_legs =
                realloc(itinerary->legs,
                        (itinerary->n_legs + 1) * sizeof(struct raptor_leg *));

        if (new_legs == NULL) {
                free(leg);
                return false;
        }

        // 4. Update struct fields
        itinerary->legs = new_legs;

        leg->from = dep;
        leg->to = arr;
        leg->transfer = transfer;
        leg->trip = trip;
        leg->dep_time = dep_time;
        leg->arr_time = arr_time;

        itinerary->legs[itinerary->n_legs++] = leg;
        if (leg->transfer != NULL)
                itinerary->n_transfers++;
        if (leg->trip != NULL)
                itinerary->n_trips++;

        return true;
}

struct raptor_itinerary **
raptor_reconstruct(struct raptor_stop *dep_stop, struct raptor_stop *arr_stop,
                   struct raptor_stop ***parent_stops,
                   struct trip ***parent_trips,
                   struct transfer ***parent_transfers,
                   unsigned long int **tau_rounds, unsigned long int *tau_star,
                   size_t total_rounds, size_t n_stops, size_t *n_itineraries)
{
        struct raptor_itinerary **itineraries = NULL;

        /*printf("Raptor algo with: %s (%s) -> %s (%s)\n",
               dep_stop->stop->stop_name, dep_stop->stop->stop_id,
               arr_stop->stop->stop_name, arr_stop->stop->stop_id); */

        // Path Reconstruction
        if (tau_star[arr_stop->idx] == UINT_MAX) {
                //printf("Destination unreachable!\n");
        } else {
                for (size_t k = 1; k < total_rounds - 1; k++) {
                        if (tau_rounds[k][arr_stop->idx] >=
                            tau_rounds[k - 1][arr_stop->idx])
                                continue;

                        /*printf("------------ Round k = %zu | tau = %zu ------------\n",
                               k, tau_rounds[k][arr_stop->idx]); */

                        itineraries = realloc(
                                itineraries,
                                (*n_itineraries + 1) *
                                        sizeof(struct raptor_itinerary *));
                        struct raptor_itinerary *itinerary =
                                calloc(1, sizeof(struct raptor_itinerary));
                        itinerary->n_legs = 0;
                        itinerary->n_transfers = 0;
                        itinerary->n_trips = 0;
                        itinerary->arr_time = tau_rounds[k][arr_stop->idx];
                        itinerary->dep_time = tau_star[dep_stop->idx];
                        itineraries[*n_itineraries] = itinerary;
                        (*n_itineraries)++;

                        struct raptor_stop *curr_stop = arr_stop;

                        // Find highest round that updated arrival at arr_stop
                        size_t curr_round = k;
                        while (curr_round > 0 &&
                               parent_stops[curr_round][curr_stop->idx] ==
                                       NULL) {
                                curr_round--;
                        }

                        while (curr_stop != NULL && curr_stop != dep_stop) {
                                if (curr_stop->idx >= n_stops)
                                        break;

                                if (parent_stops[curr_round][curr_stop->idx] ==
                                    NULL) {
                                        if (curr_round > 0) {
                                                curr_round--;
                                                continue;
                                        } else {
                                                break;
                                        }
                                }

                                struct raptor_stop *prev_stop =
                                        parent_stops[curr_round][curr_stop->idx];
                                struct trip *tr =
                                        parent_trips[curr_round][curr_stop->idx];

                                if (prev_stop == NULL)
                                        break;

                                const char *from_name =
                                        (prev_stop->stop &&
                                         prev_stop->stop->stop_name) ?
                                                prev_stop->stop->stop_name :
                                                "Unknown";
                                const char *to_name =
                                        (curr_stop->stop &&
                                         curr_stop->stop->stop_name) ?
                                                curr_stop->stop->stop_name :
                                                "Unknown";
                                const char *from_id =
                                        (prev_stop->stop &&
                                         prev_stop->stop->stop_id) ?
                                                prev_stop->stop->stop_id :
                                                "Unknown";
                                const char *to_id =
                                        (curr_stop->stop &&
                                         curr_stop->stop->stop_id) ?
                                                curr_stop->stop->stop_id :
                                                "Unknown";
                                const char *from_route_name =
                                        (prev_stop->routes_stops &&
                                         prev_stop->n_routes > 0) ?
                                                prev_stop->routes_stops[0]
                                                        ->route->route
                                                        ->route_short_name :
                                                "Unknown";
                                const char *to_route_name =
                                        (curr_stop->routes_stops &&
                                         curr_stop->n_routes > 0) ?
                                                curr_stop->routes_stops[0]
                                                        ->route->route
                                                        ->route_short_name :
                                                "Unknown";

                                if (tr != NULL) {
                                        const char *headsign =
                                                tr->trip_headsign ?
                                                        tr->trip_headsign :
                                                        "N/A";
                                        /*printf("Boarded trip %s at %s (%s) to %s (%s) on line: %s | time: %s\n",
                                               headsign, from_name, from_id,
                                               to_name, to_id,
                                               tr->route->route_short_name,
                                               print_hhmmss(
                                                       tau_rounds
                                                               [curr_round]
                                                               [prev_stop->idx]));*/
                                        raptor_itineray_add_leg(
                                                itinerary, NULL, tr, prev_stop,
                                                curr_stop,
                                                tau_rounds[curr_round]
                                                          [prev_stop->idx],
                                                tau_rounds[curr_round]
                                                          [curr_stop->idx]);

                                        curr_stop = prev_stop;
                                        if (curr_round > 0)
                                                curr_round--;
                                } else {
                                        struct transfer *ut =
                                                parent_transfers[curr_round]
                                                                [curr_stop->idx];
                                        /*printf("Transfer at %s (%s) line %s to %s (%s) line %s time:%s "
                                               "(arrived %s)\n",
                                               from_name, from_id,
                                               from_route_name, to_name, to_id,
                                               to_route_name,
                                               print_hhmmss(
                                                       tau_rounds[curr_round]
                                                                 [curr_stop->idx]),
                                               print_hhmmss(
                                                       tau_rounds
                                                               [curr_round]
                                                               [prev_stop->idx]));*/

                                        if (ut != NULL) {
                                                raptor_itineray_add_leg(
                                                        itinerary, ut, NULL,
                                                        prev_stop, curr_stop,
                                                        tau_rounds[curr_round]
                                                                  [prev_stop->idx],
                                                        tau_rounds
                                                                [curr_round]
                                                                [curr_stop->idx]);
                                        }
                                        /*printf("   [used] %s -> %s stored=%us steps=%zu\n",
                                                       ut->from_stop->stop_id,
                                                       ut->to_stop->stop_id,
                                                       ut->min_transfer_time,
                                                       ut->n_steps); */
                                        for (size_t i = 0; i < ut->n_steps;
                                             i++) {
                                                struct pathway *p =
                                                        ut->steps[i];
                                                if (p == NULL)
                                                        continue;
                                        }

                                        curr_stop = prev_stop;
                                }
                        }
                        if (curr_stop != NULL &&
                            curr_stop->idx == dep_stop->idx) {
                                /*printf("Successfully reconstructed route to %s!\n",
                                       dep_stop->stop->stop_name); */
                        } else {
                                //printf("Could not trace route back to departure stop.\n");
                                free_raptor_itinerary(itinerary);
                        }
                }
        }
        return itineraries;
}

struct trip *et(struct raptor_route *r, struct raptor_route_stop *pi,
                unsigned long int prev_tau)
{
        for (size_t i = 0; i < r->n_trips; i++) {
                struct stop_time *s = r->stop_times[i][pi->pos_in_route];

                if (s->departure_time >= prev_tau) {
                        return s->trip;
                }
        }
        return NULL;
}

struct raptor_result *
raptor_algo(const size_t n_routes, struct raptor_stop **stops,
            const size_t n_stops, const unsigned long int dep_time,
            struct raptor_stop *dep_stop, struct raptor_stop *arr_stop)
{
        /*printf("Raptor algo with: %s (%s) -> %s (%s)\n",
               dep_stop->stop->stop_name, dep_stop->stop->stop_id,
               arr_stop->stop->stop_name, arr_stop->stop->stop_id); */

        const size_t max_trips = 5;
        const size_t total_rounds = max_trips + 1;

        bool *marked = calloc(n_stops, sizeof(bool));
        bool *marked_t = calloc(n_stops, sizeof(bool));
        unsigned long int *tau_star =
                calloc(n_stops, sizeof(unsigned long int));
        unsigned long int **tau_rounds =
                calloc(total_rounds, sizeof(unsigned long int *));

        struct raptor_stop ***parent_stops =
                calloc(total_rounds, sizeof(struct raptor_stop **));
        struct trip ***parent_trips =
                calloc(total_rounds, sizeof(struct trip **));
        struct transfer ***parent_transfers =
                calloc(total_rounds, sizeof(struct transfer **));

        struct raptor_stop *boarding_stop = NULL;

        for (size_t s = 0; s < n_stops; s++) {
                tau_star[s] = UINT_MAX;
        }

        for (size_t i = 0; i < total_rounds; i++) {
                tau_rounds[i] = calloc(n_stops, sizeof(unsigned long int));
                parent_stops[i] =
                        calloc(n_stops, sizeof(struct raptor_stop **));
                parent_trips[i] = calloc(n_stops, sizeof(struct trip **));
                parent_transfers[i] =
                        calloc(n_stops, sizeof(struct transfer **));

                for (size_t j = 0; j < n_stops; j++) {
                        tau_rounds[i][j] = UINT_MAX;
                }
        }

        tau_rounds[0][dep_stop->idx] = dep_time;
        tau_star[dep_stop->idx] = dep_time;
        parent_stops[0][dep_stop->idx] = dep_stop;
        parent_trips[0][dep_stop->idx] = NULL;
        parent_transfers[0][dep_stop->idx] = NULL;

        for (size_t i = 0; i < dep_stop->n_transfers; i++) {
                struct transfer *t = dep_stop->transfers[i];
                if (t == NULL) {
                        continue;
                }
                struct raptor_stop *target = t->to_rstop;
                if (target == NULL) {
                        continue;
                }
                size_t walk_arrival = dep_time + t->min_transfer_time;
                if (walk_arrival < tau_star[target->idx]) {
                        tau_rounds[0][target->idx] = walk_arrival;
                        tau_star[target->idx] = walk_arrival;
                        parent_stops[0][target->idx] = dep_stop;
                        parent_trips[0][target->idx] =
                                NULL; // Reached via foot, not transit
                        parent_transfers[0][target->idx] =
                                t; // Track initial footpath
                        marked[target->idx] =
                                true; // Mark for Round 1 route scanning
                }
        }

        marked[dep_stop->idx] = true;

        for (size_t i = 0; i < dep_stop->n_transfers; i++) {
                struct transfer *tr = dep_stop->transfers[i];
                struct raptor_stop *pp = tr->to_rstop;
                if (pp == NULL) {
                        continue;
                }
                if (pp == NULL || pp->idx >= n_stops || pp == dep_stop ||
                    pp->idx == dep_stop->idx)
                        continue;

                unsigned short l = tr->min_transfer_time;
                if (!tr->is_inter_parent_transfer)
                        l *= 2;

                unsigned long walk_arr = dep_time + l;

                parent_transfers[0][pp->idx] = tr;
                tau_rounds[0][pp->idx] = walk_arr;
                tau_star[pp->idx] = walk_arr;
                marked[pp->idx] =
                        true; // Mark so Round 1 can board routes serving pp!
        }

        struct raptor_route_stop **Q_stops =
                calloc(n_routes, sizeof(struct raptor_route_stop *));
        struct raptor_route **Q_routes =
                calloc(n_routes, sizeof(struct raptor_route *));
        //printf("Init finished\n");

        for (size_t k = 1; k < total_rounds; k++) {
                //printf("%zu round init\n", k);
                memcpy(tau_rounds[k], tau_rounds[k - 1],
                       n_stops * sizeof(unsigned long));

                clear_array(Q_stops, n_routes,
                            sizeof(struct raptor_route_stop *));
                clear_array(Q_routes, n_routes, sizeof(struct raptor_route *));

                //printf("%zu round init finished\n", k);

                for (size_t mi = 0; mi < n_stops; mi++) {
                        if (marked[mi]) {
                                // printf("%zu\n",mi);
                                struct raptor_stop *p = stops[mi];
                                if (p == NULL)
                                        continue;
                                // printf("%zu | %s\n", mi, p->stop->stop_id);
                                for (size_t ri = 0; ri < p->n_routes; ri++) {
                                        struct raptor_route_stop *route_stop =
                                                p->routes_stops[ri];
                                        struct raptor_route *r =
                                                route_stop->route;

                                        if (r->idx >= n_routes) {
                                                /*fprintf(stderr,
                                                        "CRITICAL ERROR: r->idx (%zu) >= n_routes (%zu)\n",
                                                        r->idx, n_routes); */
                                                exit(1);
                                        }

                                        // printf("%i, %i\n", route_stop == NULL, r == NULL);

                                        if (Q_routes[r->idx] &&
                                            Q_stops[r->idx]) {
                                                if (Q_stops[r->idx]
                                                            ->pos_in_route >
                                                    route_stop->pos_in_route) {
                                                        Q_stops[r->idx] =
                                                                route_stop;
                                                }
                                        } else {
                                                Q_routes[r->idx] = r;
                                                Q_stops[r->idx] = route_stop;
                                        }
                                }
                                marked[mi] = false;
                        }
                }

                //printf("Stage 1 finished\n");

                for (size_t ri = 0; ri < n_routes; ri++) {
                        struct raptor_route *r = Q_routes[ri];
                        struct raptor_route_stop *p = Q_stops[ri];

                        if (r == NULL || p == NULL)
                                continue;

                        struct trip *t = NULL;

                        for (size_t i = p->pos_in_route; i < r->n_stops; i++) {
                                if (r->route_stops[i] == NULL) {
                                        continue;
                                }
                                struct raptor_route_stop *pi =
                                        r->route_stops[i];
                                if (pi == NULL || pi->rstop == NULL)
                                        continue;
                                if (t != NULL) {
                                        if (t->stop_times != NULL &&
                                            pi->pos_in_route <
                                                    t->stop_time_count &&
                                            t->stop_times[pi->pos_in_route] !=
                                                    NULL) {
                                                if (t->stop_times[pi->pos_in_route]
                                                            ->arrival_time <
                                                    (unsigned long)min_int(
                                                            tau_star[pi->rstop->idx],
                                                            tau_star[arr_stop->idx])) {
                                                        tau_rounds[k][pi->rstop
                                                                              ->idx] =
                                                                t->stop_times[pi->pos_in_route]
                                                                        ->arrival_time;
                                                        tau_star[pi->rstop->idx] =
                                                                t->stop_times[pi->pos_in_route]
                                                                        ->arrival_time;
                                                        // printf("%zu\n", t->stop_times[pi->pos_in_route]->arrival_time);

                                                        parent_stops
                                                                [k]
                                                                [pi->rstop->idx] =
                                                                        boarding_stop;
                                                        parent_trips
                                                                [k]
                                                                [pi->rstop->idx] =
                                                                        t;

                                                        marked[pi->rstop->idx] =
                                                                true;
                                                }
                                        }
                                }
                                if (tau_rounds[k - 1][pi->rstop->idx] <
                                    UINT_MAX) {
                                        bool can_board = false;

                                        if (t == NULL) {
                                                can_board = true;
                                        } else if (t->stop_times != NULL &&
                                                   pi->pos_in_route <
                                                           t->stop_time_count &&
                                                   t->stop_times[pi->pos_in_route] !=
                                                           NULL) {
                                                can_board =
                                                        (tau_rounds
                                                                 [k - 1]
                                                                 [pi->rstop->idx] <=
                                                         t->stop_times[pi->pos_in_route]
                                                                 ->departure_time);
                                        }

                                        if (can_board) {
                                                struct trip *t_temp = et(
                                                        r, pi,
                                                        tau_rounds
                                                                [k - 1]
                                                                [pi->rstop->idx]);
                                                if (t_temp != NULL) {
                                                        t = t_temp;
                                                        boarding_stop =
                                                                pi->rstop;
                                                }
                                        }
                                }
                        }
                        boarding_stop = NULL;
                }

                memcpy(marked_t, marked, n_stops * sizeof(bool));

                for (size_t mi = 0; mi < n_stops; mi++) {
                        if (marked_t[mi]) {
                                struct raptor_stop *p = stops[mi];
                                if (p == NULL) {
                                        continue;
                                }

                                for (size_t i = 0; i < p->n_transfers; i++) {
                                        struct transfer *t = p->transfers[i];
                                        if (t == NULL)
                                                continue;
                                        struct raptor_stop *pp = t->to_rstop;
                                        if (pp == NULL || t == NULL) {
                                                continue;
                                        }
                                        if (pp->idx >= n_stops) {
                                                // fprintf(stderr, "CRITICAL ERROR: pp->idx (%zu) >= n_stops
                                                // (%zu)\n", pp->idx, n_stops);
                                                exit(1);
                                        }

                                        // printf("Tranfsers: %s (%s) -> %s (%s)\n", t->from_stop->stop_name,
                                        // t->from_stop->stop_id, t->to_rstop->stop->stop_name,
                                        // t->to_rstop->stop->stop_id);

                                        unsigned short l = t->min_transfer_time;
                                        if (t->is_inter_parent_transfer ==
                                            false)
                                                l *= 2;
                                        l += 10;

                                        unsigned long prev_val =
                                                tau_rounds[k][pp->idx];

                                        tau_rounds[k][pp->idx] = min_int(
                                                tau_rounds[k][pp->idx],
                                                tau_rounds[k][p->idx] + l);
                                        if (tau_rounds[k][pp->idx] <
                                            tau_star[pp->idx]) {
                                                tau_star[pp->idx] =
                                                        tau_rounds[k][pp->idx];
                                        }
                                        if (prev_val !=
                                            tau_rounds[k][pp->idx]) {
                                                parent_stops[k][pp->idx] = p;
                                                parent_trips[k][pp->idx] = NULL;
                                                parent_transfers[k][pp->idx] =
                                                        t;
                                                marked[pp->idx] = true;
                                        }
                                }
                        }
                }
        }
        struct raptor_result *res = calloc(1, sizeof(struct raptor_result));
        res->parent_stops = parent_stops;
        res->parent_transfers = parent_transfers;
        res->parent_trips = parent_trips;
        res->tau_star = tau_star;
        res->tau_rounds = tau_rounds;
        res->total_rounds = total_rounds;
        res->n_itineraries = 0;
        res->dep = dep_stop;
        res->arr = arr_stop;
        res->departure_time = dep_time;

        struct raptor_itinerary **itineraries = raptor_reconstruct(
                dep_stop, arr_stop, parent_stops, parent_trips,
                parent_transfers, tau_rounds, tau_star, total_rounds, n_stops,
                &res->n_itineraries);
        res->itineraries = itineraries;

        free(marked);
        free(marked_t);
        free(Q_stops);
        free(Q_routes);
        return res;
}

void format_raptor(struct raptor_result *res)
{
        if (res == NULL || res->dep == NULL || res->arr == NULL ||
            res->itineraries == NULL) {
                return;
        }

        printf("Raptor Algo Results | %s (%s) -> %s (%s) | Departure Time : %s | Found itineraries : %zu\n",
               res->dep->stop->stop_name, res->dep->stop->stop_id,
               res->arr->stop->stop_name, res->arr->stop->stop_id,
               print_hhmmss(res->departure_time), res->n_itineraries);
        for (size_t i = 0; i < res->n_itineraries; i++) {
                struct raptor_itinerary *itinerary = res->itineraries[i];
                if (itinerary == NULL) {
                        printf("Itineray %zu is not valid\n", i);
                        continue;
                }
                printf("------------- Itinerary %zu | %zu trips -------------\n",
                       i, itinerary->n_trips);
                for (size_t j = itinerary->n_legs; j > 0; j--) {
                        struct raptor_leg *leg = itinerary->legs[j - 1];
                        if (leg == NULL || leg->from == NULL ||
                            leg->to == NULL || leg->from->stop == NULL ||
                            leg->to->stop == NULL) {
                                printf("leg %zu is not valid\n", j - 1);
                                continue;
                        }
                        if (leg->trip != NULL) {
                                printf("%s || %s (%s)\n         || %s - %s \n         ||\n%s || %s (%s)\n",
                                       print_hhmmss(leg->dep_time),
                                       leg->from->stop->stop_name,
                                       leg->from->stop->stop_id,
                                       leg->trip->route->route_short_name,
                                       leg->trip->trip_headsign,
                                       print_hhmmss(leg->arr_time),
                                       leg->to->stop->stop_name,
                                       leg->to->stop->stop_id);
                        }
                        if (leg->transfer != NULL) {
                                printf("Transfer | %hu min \n",
                                       (leg->transfer->min_transfer_time / 60));
                        }
                }
        }
}

void free_raptor_legs(struct raptor_leg **legs, size_t n_legs)
{
        for (size_t i = 0; i < n_legs; i++) {
                struct raptor_leg *l = legs[i];
                free(l);
        }
        free(legs);
}

void free_raptor_itinerary(struct raptor_itinerary *itinerary)
{
        if (itinerary != NULL && itinerary->legs != NULL) {
                free_raptor_legs(itinerary->legs, itinerary->n_legs);
        }
        free(itinerary);
}

void free_raptor_itineraries(struct raptor_itinerary **itineraries,
                             size_t n_itineraries)
{
        if (itineraries == NULL)
                return;
        for (size_t i = 0; i < n_itineraries; i++) {
                struct raptor_itinerary *itinerary = itineraries[i];
                if (itineraries != NULL)
                        free_raptor_itinerary(itinerary);
        }
        free(itineraries);
}

void free_raptor_data(struct raptor_route **raptor_routes,
                      size_t n_raptor_routes, struct raptor_stop **raptor_stops,
                      size_t n_raptor_stops,
                      struct raptor_result *raptor_result)
{
        for (size_t i = 0; i < n_raptor_routes; i++) {
                struct raptor_route *r = raptor_routes[i];
                if (r == NULL)
                        continue;

                if (r->route_stops != NULL) {
                        for (size_t j = 0; j < r->n_stops; j++) {
                                struct raptor_route_stop *rrs =
                                        r->route_stops[j];
                                if (rrs == NULL)
                                        continue;
                                free(rrs);
                        }
                }
                free(r->route_stops);
                for (size_t j = 0; j < r->n_trips; j++) {
                        free(r->stop_times[j]);
                }

                free(r->stop_times);
                free(r);
        }
        for (size_t i = 0; i < n_raptor_stops; i++) {
                struct raptor_stop *rs = raptor_stops[i];
                if (rs == NULL)
                        continue;
                free(rs->routes_stops);
                free(rs->transfers);
                free(rs);
        }
        if (raptor_result != NULL) {
                if (raptor_result->parent_stops != NULL)
                        free(raptor_result->parent_stops);
                if (raptor_result->parent_transfers != NULL)
                        free(raptor_result->parent_transfers);
                if (raptor_result->parent_trips != NULL)
                        free(raptor_result->parent_trips);
                if (raptor_result->tau_rounds) {
                        for (size_t i = 0; i < raptor_result->total_rounds;
                             i++) {
                                free(raptor_result->tau_rounds[i]);
                        }
                        free(raptor_result->tau_rounds);
                }
                if (raptor_result->tau_star != NULL)
                        free(raptor_result->tau_star);
        }
}

void free_raptor_result(struct raptor_result *raptor_result,
                        bool with_itineraries)
{
        if (raptor_result == NULL)
                return;
        if (raptor_result->parent_stops != NULL)
                free(raptor_result->parent_stops);
        if (raptor_result->parent_transfers != NULL)
                free(raptor_result->parent_transfers);
        if (raptor_result->parent_trips != NULL)
                free(raptor_result->parent_trips);
        if (raptor_result->tau_rounds) {
                for (size_t i = 0; i < raptor_result->total_rounds; i++) {
                        free(raptor_result->tau_rounds[i]);
                }
                free(raptor_result->tau_rounds);
        }
        if (raptor_result->tau_star != NULL)
                free(raptor_result->tau_star);
        if (raptor_result->itineraries != NULL && with_itineraries)
                free_raptor_itineraries(raptor_result->itineraries,
                                        raptor_result->n_itineraries);
}

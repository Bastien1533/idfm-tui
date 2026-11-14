#include "algo/raptor.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data/gtfs.h"
#include "utils/utils.h"

char *get_trip_sequence_key(struct trip *trip)
{
    size_t total_len = 0;
    for (size_t i = 0; i < trip->stop_time_count; i++)
    {
        if (trip->stop_times && trip->stop_times[i]->stop
            && trip->stop_times[i]->stop->stop_name)
        {
            total_len += strlen(trip->stop_times[i]->stop->stop_name);
        }
    }

    char *dest = malloc((total_len + 1) * sizeof(char));

    char *curr = dest;
    for (size_t i = 0; i < trip->stop_time_count; i++)
    {
        if (trip->stop_times && trip->stop_times[i]->stop
            && trip->stop_times[i]->stop->stop_name)
        {
            size_t len = strlen(trip->stop_times[i]->stop->stop_name);
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
    if (n_trips == 0)
    {
        return;
    }
    for (size_t i = n_trips - 1; i > 0; i--)
    {
        bool sorted = true;
        for (size_t j = 0; j < i; j++)
        {
            if (stop_times[j + 1][0]->departure_time
                < stop_times[j][0]->departure_time)
            {
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

    for (size_t i = 0; i < n_trips; i++)
    {
        struct trip *t = trips[i];
        char *seq = get_trip_sequence_key(t);

        struct raptor_route *route = hash_map_get(map, seq);
        if (route == NULL)
        {
            struct raptor_route *r = malloc(sizeof(struct raptor_route));
            r->route = t->route;
            r->stop_times = calloc(1, sizeof(struct stop_time **));
            r->stop_times[0] =
                calloc(t->stop_time_count, sizeof(struct stop_time *));
            r->n_trips = 1;
            r->route_stops =
                calloc(t->stop_time_count, sizeof(struct raptor_route_stop *));
            r->n_stops = t->stop_time_count;
            for (size_t i = 0; i < t->stop_time_count; i++)
            {
                if (t->stop_times && t->stop_times[i]->stop
                    && t->stop_times[i]->stop->stop_name)
                {
                    r->route_stops[i] =
                        malloc(sizeof(struct raptor_route_stop));
                    r->route_stops[i]->stop = t->stop_times[i]->stop;
                    r->route_stops[i]->pos_in_route = i;
                    r->route_stops[i]->route = r;

                    r->stop_times[0][i] = t->stop_times[i];
                }
            }
            hash_map_insert(map, seq, r);
        }
        else
        {
            route->stop_times =
                realloc(route->stop_times,
                        (route->n_trips + 1) * sizeof(struct stop_time **));
            route->n_trips++;
            route->stop_times[route->n_trips - 1] =
                calloc(t->stop_time_count, sizeof(struct stop_time *));
            for (size_t i = 0; i < t->stop_time_count; i++)
            {
                if (t->stop_times && t->stop_times[i]->stop
                    && t->stop_times[i]->stop->stop_name)
                {
                    route->stop_times[route->n_trips - 1][i] = t->stop_times[i];
                }
            }
        }

        free(seq);
    }

    struct raptor_route **routes =
        calloc(map->size, sizeof(struct raptor_route *));

    size_t idx = 0;
    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->nodes[i] != NULL)
        {
            struct hash_node *curr = map->nodes[i];
            while (curr != NULL)
            {
                struct hash_node *next = curr->next;
                struct raptor_route *r = curr->value;
                if (r)
                {
                    if (r->stop_times == NULL)
                    {
                        printf("fuck\n");
                    }
                    else
                    {
                        sort_stop_time_blocks_bubble(r->stop_times, r->n_trips);
                        routes[idx] = r;
                        r->idx = idx;
                        idx++;
                    }
                }
                curr = next;
            }
        }
    }
    if (n_routes != NULL)
    {
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
    for (size_t i = 0; i < n_all_stops; i++)
    {
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
        if (s->transfer_count > 0)
        {
            rs->transfers =
                calloc(s->transfer_count, sizeof(struct transfer *));
            for (size_t x = 0; x < s->transfer_count; x++)
            {
                rs->transfers[x] = s->transfers[x];
            }
        }
        else
        {
            rs->transfers = NULL;
        }

        stops[i] = rs;
        hash_map_insert(map, s->stop_id, rs);
    }

    // 2. Link routes to existing raptor_stops
    for (size_t i = 0; i < n_routes; i++)
    {
        struct raptor_route *r = routes[i];
        for (size_t j = 0; j < r->n_stops; j++)
        {
            struct raptor_route_stop *s = r->route_stops[j];
            if (s == NULL || s->stop == NULL)
                continue;

            struct raptor_stop *rs = hash_map_get(map, s->stop->stop_id);
            if (rs != NULL)
            {
                rs->n_routes++;
                rs->routes_stops =
                    realloc(rs->routes_stops,
                            rs->n_routes * sizeof(struct raptor_route_stop *));
                rs->routes_stops[rs->n_routes - 1] = s;
                s->rstop = rs;
            }
        }
    }

    for (size_t i = 0; i < n_all_stops; i++)
    {
        struct raptor_stop *rs = stops[i];
        if (rs == NULL)
            continue;

        for (size_t ti = 0; ti < rs->n_transfers; ti++)
        {
            if (rs->transfers[ti] != NULL && rs->transfers[ti]->to_stop != NULL)
            {
                struct raptor_stop *to_rstop =
                    hash_map_get(map, rs->transfers[ti]->to_stop->stop_id);
                rs->transfers[ti]->to_rstop = to_rstop;
            }
        }
    }

    if (n_raptor_stops != NULL)
    {
        *n_raptor_stops = n_all_stops;
    }

    free_hashmap(map);
    return stops;
}

struct hash_map *raptor_stops_to_map(struct raptor_stop **stops,
                                     size_t *n_raptor_stops)
{
    if (stops == NULL)
    {
        return NULL;
    }
    struct hash_map *map = init_hashmap(*n_raptor_stops);
    for (size_t i = 0; i < *n_raptor_stops; i++)
    {
        struct raptor_stop *s = stops[i];
        if (s == NULL)
        {
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
    if (itinerary == NULL || dep == NULL || arr == NULL
        || (trip == NULL && transfer == NULL))
    {
        return false;
    }

    struct raptor_leg *leg = calloc(1, sizeof(struct raptor_leg));
    if (leg == NULL)
    {
        return false;
    }

    struct raptor_leg **new_legs = realloc(
        itinerary->legs, (itinerary->n_legs + 1) * sizeof(struct raptor_leg *));

    if (new_legs == NULL)
    {
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
    if (tau_star[arr_stop->idx] == UINT_MAX)
    {
        // printf("Destination unreachable!\n");
    }
    else
    {
        for (size_t k = 1; k < total_rounds - 1; k++)
        {
            if (tau_rounds[k][arr_stop->idx]
                >= tau_rounds[k - 1][arr_stop->idx])
                continue;

            /*printf("------------ Round k = %zu | tau = %zu ------------\n",
                   k, tau_rounds[k][arr_stop->idx]); */

            itineraries = realloc(itineraries,
                                  (*n_itineraries + 1)
                                      * sizeof(struct raptor_itinerary *));
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
            while (curr_round > 0
                   && parent_stops[curr_round][curr_stop->idx] == NULL)
            {
                curr_round--;
            }

            while (curr_stop != NULL && curr_stop != dep_stop)
            {
                if (curr_stop->idx >= n_stops)
                    break;

                if (parent_stops[curr_round][curr_stop->idx] == NULL)
                {
                    if (curr_round > 0)
                    {
                        curr_round--;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }

                struct raptor_stop *prev_stop =
                    parent_stops[curr_round][curr_stop->idx];
                struct trip *tr = parent_trips[curr_round][curr_stop->idx];

                if (prev_stop == NULL)
                    break;

                const char *from_name =
                    (prev_stop->stop && prev_stop->stop->stop_name)
                    ? prev_stop->stop->stop_name
                    : "Unknown";
                const char *to_name =
                    (curr_stop->stop && curr_stop->stop->stop_name)
                    ? curr_stop->stop->stop_name
                    : "Unknown";
                const char *from_id =
                    (prev_stop->stop && prev_stop->stop->stop_id)
                    ? prev_stop->stop->stop_id
                    : "Unknown";
                const char *to_id =
                    (curr_stop->stop && curr_stop->stop->stop_id)
                    ? curr_stop->stop->stop_id
                    : "Unknown";
                const char *from_route_name =
                    (prev_stop->routes_stops && prev_stop->n_routes > 0)
                    ? prev_stop->routes_stops[0]->route->route->route_short_name
                    : "Unknown";
                const char *to_route_name =
                    (curr_stop->routes_stops && curr_stop->n_routes > 0)
                    ? curr_stop->routes_stops[0]->route->route->route_short_name
                    : "Unknown";

                if (tr != NULL)
                {
                    const char *headsign =
                        tr->trip_headsign ? tr->trip_headsign : "N/A";
                    /*printf("Boarded trip %s at %s (%s) to %s (%s) on line: %s
                       | time: %s\n", headsign, from_name, from_id, to_name,
                       to_id, tr->route->route_short_name, print_hhmmss(
                                   tau_rounds
                                           [curr_round]
                                           [prev_stop->idx]));*/
                    raptor_itineray_add_leg(
                        itinerary, NULL, tr, prev_stop, curr_stop,
                        tau_rounds[curr_round][prev_stop->idx],
                        tau_rounds[curr_round][curr_stop->idx]);

                    curr_stop = prev_stop;
                    if (curr_round > 0)
                        curr_round--;
                }
                else
                {
                    struct transfer *ut =
                        parent_transfers[curr_round][curr_stop->idx];
                    /*printf("Transfer at %s (%s) line %s to %s (%s) line %s
                       time:%s "
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

                    if (ut != NULL)
                    {
                        raptor_itineray_add_leg(
                            itinerary, ut, NULL, prev_stop, curr_stop,
                            tau_rounds[curr_round][prev_stop->idx],
                            tau_rounds[curr_round][curr_stop->idx]);
                    }
                    /*printf("   [used] %s -> %s stored=%us steps=%zu\n",
                                   ut->from_stop->stop_id,
                                   ut->to_stop->stop_id,
                                   ut->min_transfer_time,
                                   ut->n_steps); */
                    for (size_t i = 0; i < ut->n_steps; i++)
                    {
                        struct pathway *p = ut->steps[i];
                        if (p == NULL)
                            continue;
                    }

                    curr_stop = prev_stop;
                }
            }
            if (curr_stop != NULL && curr_stop->idx == dep_stop->idx)
            {
                /*printf("Successfully reconstructed route to %s!\n",
                       dep_stop->stop->stop_name); */
            }
            else
            {
                // printf("Could not trace route back to departure stop.\n");
                free_raptor_itinerary(itinerary);
            }
        }
    }
    return itineraries;
}

struct trip *et(struct raptor_route *r, struct raptor_route_stop *pi,
                unsigned long int prev_tau)
{
    for (size_t i = 0; i < r->n_trips; i++)
    {
        struct stop_time *s = r->stop_times[i][pi->pos_in_route];

        if (s->departure_time >= prev_tau)
        {
            return s->trip;
        }
    }
    return NULL;
}

/* Internal algorithm state across all rounds */
struct raptor_algo_state
{
    size_t total_rounds;
    size_t n_stops;
    size_t n_routes;

    unsigned long int *tau_star; /* Best known arrival time at each stop */
    unsigned long int **tau_rounds; /* Arrival time at each stop per round k */
    struct raptor_stop ***parent_stops; /* Boarding stop leading to this stop */
    struct trip ***parent_trips; /* Trip used to reach this stop */
    struct transfer ***parent_transfers; /* Transfer used to reach this stop */

    bool *marked; /* Stops updated in current phase */
    bool *marked_t; /* Snapshot of marked stops for transfers */
    struct raptor_route_stop **q_stops; /* Earliest marked stop on each route */
    struct raptor_route *
        *q_routes; /* Routes to be processed in current round */
};

/*
 * Allocate algorithm matrices and initialize arrival times to infinity
 * (UINT_MAX). Set the departure time at the origin stop for round 0.
 */
static struct raptor_algo_state *
raptor_state_init(size_t n_routes, size_t n_stops, size_t max_trips,
                  struct raptor_stop *dep_stop, unsigned long dep_time)
{
    struct raptor_algo_state *state =
        calloc(1, sizeof(struct raptor_algo_state));
    state->total_rounds = max_trips + 1;
    state->n_stops = n_stops;
    state->n_routes = n_routes;

    state->marked = calloc(n_stops, sizeof(bool));
    state->marked_t = calloc(n_stops, sizeof(bool));
    state->tau_star = calloc(n_stops, sizeof(unsigned long int));
    state->tau_rounds =
        calloc(state->total_rounds, sizeof(unsigned long int *));

    state->parent_stops =
        calloc(state->total_rounds, sizeof(struct raptor_stop **));
    state->parent_trips = calloc(state->total_rounds, sizeof(struct trip **));
    state->parent_transfers =
        calloc(state->total_rounds, sizeof(struct transfer **));

    state->q_stops = calloc(n_routes, sizeof(struct raptor_route_stop *));
    state->q_routes = calloc(n_routes, sizeof(struct raptor_route *));

    for (size_t s = 0; s < n_stops; s++)
    {
        state->tau_star[s] = UINT_MAX;
    }

    for (size_t k = 0; k < state->total_rounds; k++)
    {
        state->tau_rounds[k] = calloc(n_stops, sizeof(unsigned long int));
        state->parent_stops[k] = calloc(n_stops, sizeof(struct raptor_stop *));
        state->parent_trips[k] = calloc(n_stops, sizeof(struct trip *));
        state->parent_transfers[k] = calloc(n_stops, sizeof(struct transfer *));

        for (size_t s = 0; s < n_stops; s++)
        {
            state->tau_rounds[k][s] = UINT_MAX;
        }
    }

    /* Set origin stop at round 0 */
    state->tau_rounds[0][dep_stop->idx] = dep_time;
    state->tau_star[dep_stop->idx] = dep_time;
    state->parent_stops[0][dep_stop->idx] = dep_stop;
    state->marked[dep_stop->idx] = true;

    return state;
}

/*
 * Round 0: Relax direct footpaths available from the departure stop.
 * Updates tau_rounds[0] and marks reached targets for Round 1 route scanning.
 */
static void raptor_relax_origin_footpaths(struct raptor_algo_state *state,
                                          struct raptor_stop *dep_stop,
                                          unsigned long dep_time)
{
    for (size_t i = 0; i < dep_stop->n_transfers; i++)
    {
        struct transfer *tr = dep_stop->transfers[i];
        if (tr == NULL || tr->to_rstop == NULL)
            continue;

        struct raptor_stop *target = tr->to_rstop;
        if (target->idx >= state->n_stops || target == dep_stop)
            continue;

        unsigned short walk_time = tr->min_transfer_time;
        if (!tr->is_inter_parent_transfer)
            walk_time *= 2;

        unsigned long walk_arrival = dep_time + walk_time;

        if (walk_arrival < state->tau_star[target->idx])
        {
            state->tau_rounds[0][target->idx] = walk_arrival;
            state->tau_star[target->idx] = walk_arrival;
            state->parent_stops[0][target->idx] = dep_stop;
            state->parent_transfers[0][target->idx] = tr;
            state->marked[target->idx] = true;

            /* If departure transfer reached a parent station, relax to sibling
             * platforms */
            if (target->stop != NULL && target->stop->location_type == STATION)
            {
                for (size_t j = 0; j < target->n_transfers; j++)
                {
                    struct transfer *t2 = target->transfers[j];
                    if (t2 == NULL || t2->to_rstop == NULL)
                        continue;

                    struct raptor_stop *s2 = t2->to_rstop;
                    if (s2 == dep_stop || s2->idx == dep_stop->idx
                        || s2->idx >= state->n_stops)
                        continue;

                    unsigned short walk2 = t2->min_transfer_time;
                    if (!t2->is_inter_parent_transfer)
                        walk2 *= 2;

                    unsigned long s2_arr = walk_arrival + walk2;
                    if (s2_arr < state->tau_star[s2->idx])
                    {
                        state->tau_rounds[0][s2->idx] = s2_arr;
                        state->tau_star[s2->idx] = s2_arr;
                        state->parent_stops[0][s2->idx] = target;
                        state->parent_transfers[0][s2->idx] = t2;
                        state->marked[s2->idx] = true;
                    }
                }
            }
        }
    }
}

/*
 * Stage 1: Route Collection.
 * Find all routes passing through marked stops. For each route,
 * keep only the earliest boarding stop in Q_routes / Q_stops.
 */
static void raptor_stage1_collect_routes(struct raptor_algo_state *state,
                                         struct raptor_stop **stops)
{
    clear_array(state->q_stops, state->n_routes,
                sizeof(struct raptor_route_stop *));
    clear_array(state->q_routes, state->n_routes,
                sizeof(struct raptor_route *));

    for (size_t mi = 0; mi < state->n_stops; mi++)
    {
        if (!state->marked[mi])
            continue;

        struct raptor_stop *p = stops[mi];
        if (p == NULL)
            continue;

        for (size_t ri = 0; ri < p->n_routes; ri++)
        {
            struct raptor_route_stop *route_stop = p->routes_stops[ri];
            struct raptor_route *r = route_stop->route;

            /* Check if route is already queued; retain the earlier stop */
            if (state->q_routes[r->idx] && state->q_stops[r->idx])
            {
                if (state->q_stops[r->idx]->pos_in_route
                    > route_stop->pos_in_route)
                {
                    state->q_stops[r->idx] = route_stop;
                }
            }
            else
            {
                state->q_routes[r->idx] = r;
                state->q_stops[r->idx] = route_stop;
            }
        }

        /* Unmark stop after scanning its routes */
        state->marked[mi] = false;
    }
}

/*
 * Stage 2: Route Traversal.
 * Traverse each queued route from its earliest boarding stop.
 * Find the earliest trip 'et()' that departs after tau_rounds[k-1] and update
 * arrival times.
 */
static void raptor_stage2_traverse_routes(struct raptor_algo_state *state,
                                          size_t k,
                                          struct raptor_stop *arr_stop)
{
    for (size_t ri = 0; ri < state->n_routes; ri++)
    {
        struct raptor_route *r = state->q_routes[ri];
        struct raptor_route_stop *p = state->q_stops[ri];

        if (r == NULL || p == NULL)
            continue;

        struct trip *t = NULL;
        struct raptor_stop *boarding_stop = NULL;

        for (size_t i = p->pos_in_route; i < r->n_stops; i++)
        {
            struct raptor_route_stop *pi = r->route_stops[i];
            if (pi == NULL || pi->rstop == NULL)
                continue;

            /* If currently aboard a trip, check if it improves arrival at pi */
            if (t != NULL && t->stop_times != NULL
                && pi->pos_in_route < t->stop_time_count
                && t->stop_times[pi->pos_in_route] != NULL)
            {
                unsigned long arr_time =
                    t->stop_times[pi->pos_in_route]->arrival_time;
                unsigned long target_best =
                    (unsigned long)min_int(state->tau_star[pi->rstop->idx],
                                           state->tau_star[arr_stop->idx]);

                if (arr_time < target_best)
                {
                    state->tau_rounds[k][pi->rstop->idx] = arr_time;
                    state->tau_star[pi->rstop->idx] = arr_time;

                    state->parent_stops[k][pi->rstop->idx] = boarding_stop;
                    state->parent_trips[k][pi->rstop->idx] = t;
                    state->marked[pi->rstop->idx] = true;
                }
            }

            /* Check if a new or earlier trip can be boarded at pi from round
             * k-1 */
            if (state->tau_rounds[k - 1][pi->rstop->idx] < UINT_MAX)
            {
                bool can_board = false;

                if (t == NULL)
                {
                    can_board = true;
                }
                else if (t->stop_times != NULL
                         && pi->pos_in_route < t->stop_time_count
                         && t->stop_times[pi->pos_in_route] != NULL)
                {
                    can_board =
                        (state->tau_rounds[k - 1][pi->rstop->idx]
                         <= t->stop_times[pi->pos_in_route]->departure_time);
                }

                if (can_board)
                {
                    struct trip *t_temp =
                        et(r, pi, state->tau_rounds[k - 1][pi->rstop->idx]);
                    if (t_temp != NULL)
                    {
                        t = t_temp;
                        boarding_stop = pi->rstop;
                    }
                }
            }
        }
    }
}

static bool has_direct_transfer(const struct raptor_stop *from,
                                const struct raptor_stop *to)
{
    for (size_t i = 0; i < from->n_transfers; i++)
    {
        struct transfer *t = from->transfers[i];
        if (t != NULL && t->to_rstop == to)
        {
            return true;
        }
    }
    return false;
}

/*
 * Stage 3: Footpath Transfers.
 * For all stops updated by transit in Stage 2, relax foot transfers to target
 * stops.
 */
static void raptor_stage3_relax_transfers(struct raptor_algo_state *state,
                                          size_t k, struct raptor_stop **stops)
{
    /* Take snapshot of stops marked by transit in Stage 2 */
    memcpy(state->marked_t, state->marked, state->n_stops * sizeof(bool));

    for (size_t mi = 0; mi < state->n_stops; mi++)
    {
        if (!state->marked_t[mi])
            continue;

        struct raptor_stop *p = stops[mi];
        if (p == NULL)
            continue;

        for (size_t i = 0; i < p->n_transfers; i++)
        {
            struct transfer *t = p->transfers[i];
            if (t == NULL || t->to_rstop == NULL)
                continue;

            struct raptor_stop *pp = t->to_rstop;
            if (pp->idx >= state->n_stops)
                continue;

            unsigned short walk_time = t->min_transfer_time;
            if (!t->is_inter_parent_transfer)
                walk_time *= 2;
            walk_time += 10;

            unsigned long walk_arr = state->tau_rounds[k][p->idx] + walk_time;
            unsigned long prev_val = state->tau_rounds[k][pp->idx];

            state->tau_rounds[k][pp->idx] = min_int(prev_val, walk_arr);

            if (state->tau_rounds[k][pp->idx] < state->tau_star[pp->idx])
            {
                state->tau_star[pp->idx] = state->tau_rounds[k][pp->idx];
            }

            /* Record footpath parent if arrival time was updated */
            if (prev_val != state->tau_rounds[k][pp->idx])
            {
                state->parent_stops[k][pp->idx] = p;
                state->parent_trips[k][pp->idx] = NULL;
                state->parent_transfers[k][pp->idx] = t;
                state->marked[pp->idx] = true;
                /* If target is a parent station, relax transfers to sibling
                 * platforms (s2) */
                if (pp->stop != NULL && pp->stop->location_type == STATION)
                {
                    for (size_t j = 0; j < pp->n_transfers; j++)
                    {
                        struct transfer *t2 = pp->transfers[j];
                        if (t2 == NULL || t2->to_rstop == NULL)
                            continue;

                        struct raptor_stop *s2 = t2->to_rstop;
                        /* Don't walk back to origin platform p or out-of-bounds
                         * stops */
                        if (s2 == p || s2->idx == p->idx
                            || s2->idx >= state->n_stops)
                            continue;

                        if (has_direct_transfer(p, s2))
                            continue;

                        unsigned short walk2 = t2->min_transfer_time;
                        if (!t2->is_inter_parent_transfer)
                            walk2 *= 2;
                        walk2 += 60;

                        unsigned long s2_arr =
                            state->tau_rounds[k][pp->idx] + walk2;
                        unsigned long prev_s2 = state->tau_rounds[k][s2->idx];

                        if (s2_arr < prev_s2)
                        {
                            state->tau_rounds[k][s2->idx] = s2_arr;
                            if (s2_arr < state->tau_star[s2->idx])
                                state->tau_star[s2->idx] = s2_arr;

                            state->parent_stops[k][s2->idx] = pp;
                            state->parent_trips[k][s2->idx] = NULL;
                            state->parent_transfers[k][s2->idx] = t2;
                            state->marked[s2->idx] = true;
                        }
                    }
                }
            }
        }
    }
}

/*
 * Free internal working buffers used by the algorithm.
 */
static void raptor_state_free(struct raptor_algo_state *state)
{
    free(state->marked);
    free(state->marked_t);
    free(state->q_stops);
    free(state->q_routes);
    free(state);
}

/*
 * Main RAPTOR algorithm entry point.
 */
struct raptor_result *
raptor_algo(const size_t n_routes, struct raptor_stop **stops,
            const size_t n_stops, const unsigned long int dep_time,
            struct raptor_stop *dep_stop, struct raptor_stop *arr_stop)
{
    const size_t max_trips = 5;

    /* 1. Allocate matrices and set origin */
    struct raptor_algo_state *state =
        raptor_state_init(n_routes, n_stops, max_trips, dep_stop, dep_time);

    /* 2. Round 0: Relax origin footpaths */
    raptor_relax_origin_footpaths(state, dep_stop, dep_time);

    /* 3. Main round iteration (1 to max_trips) */
    for (size_t k = 1; k < state->total_rounds; k++)
    {
        /* Copy arrival times from round k-1 */
        memcpy(state->tau_rounds[k], state->tau_rounds[k - 1],
               n_stops * sizeof(unsigned long));

        /* Stage 1: Collect routes to traverse */
        raptor_stage1_collect_routes(state, stops);

        /* Stage 2: Traverse routes and update transit arrivals */
        raptor_stage2_traverse_routes(state, k, arr_stop);

        /* Stage 3: Relax footpaths from newly updated stops */
        raptor_stage3_relax_transfers(state, k, stops);
    }

    /* 4. Pack result and reconstruct path */
    struct raptor_result *res = calloc(1, sizeof(struct raptor_result));
    res->parent_stops = state->parent_stops;
    res->parent_transfers = state->parent_transfers;
    res->parent_trips = state->parent_trips;
    res->tau_star = state->tau_star;
    res->tau_rounds = state->tau_rounds;
    res->total_rounds = state->total_rounds;
    res->n_itineraries = 0;
    res->dep = dep_stop;
    res->arr = arr_stop;
    res->departure_time = dep_time;

    res->itineraries = raptor_reconstruct(
        dep_stop, arr_stop, res->parent_stops, res->parent_trips,
        res->parent_transfers, res->tau_rounds, res->tau_star,
        res->total_rounds, n_stops, &res->n_itineraries);

    raptor_state_free(state);
    return res;
}

void format_raptor(struct raptor_result *res)
{
    if (res == NULL || res->dep == NULL || res->arr == NULL
        || res->itineraries == NULL)
    {
        return;
    }

    printf("Raptor Algo Results | %s (%s) -> %s (%s) | Departure Time : %s | "
           "Found itineraries : %zu\n",
           res->dep->stop->stop_name, res->dep->stop->stop_id,
           res->arr->stop->stop_name, res->arr->stop->stop_id,
           print_hhmmss(res->departure_time), res->n_itineraries);
    for (size_t i = 0; i < res->n_itineraries; i++)
    {
        struct raptor_itinerary *itinerary = res->itineraries[i];
        if (itinerary == NULL)
        {
            printf("Itineray %zu is not valid\n", i);
            continue;
        }
        printf("------------- Itinerary %zu | %zu trips -------------\n", i,
               itinerary->n_trips);
        for (size_t j = itinerary->n_legs; j > 0; j--)
        {
            struct raptor_leg *leg = itinerary->legs[j - 1];
            if (leg == NULL || leg->from == NULL || leg->to == NULL
                || leg->from->stop == NULL || leg->to->stop == NULL)
            {
                printf("leg %zu is not valid\n", j - 1);
                continue;
            }
            if (leg->trip != NULL)
            {
                printf("%s || %s (%s)\n         || %s - %s \n         ||\n%s "
                       "|| %s (%s)\n",
                       print_hhmmss(leg->dep_time), leg->from->stop->stop_name,
                       leg->from->stop->stop_id,
                       leg->trip->route->route_short_name,
                       leg->trip->trip_headsign, print_hhmmss(leg->arr_time),
                       leg->to->stop->stop_name, leg->to->stop->stop_id);
            }
            if (leg->transfer != NULL)
            {
                printf("Transfer | %hu min \n",
                       (leg->transfer->min_transfer_time / 60));
            }
        }
    }
}

void free_raptor_legs(struct raptor_leg **legs, size_t n_legs)
{
    for (size_t i = 0; i < n_legs; i++)
    {
        struct raptor_leg *l = legs[i];
        free(l);
    }
    free(legs);
}

void free_raptor_itinerary(struct raptor_itinerary *itinerary)
{
    if (itinerary != NULL && itinerary->legs != NULL)
    {
        free_raptor_legs(itinerary->legs, itinerary->n_legs);
    }
    free(itinerary);
}

void free_raptor_itineraries(struct raptor_itinerary **itineraries,
                             size_t n_itineraries)
{
    if (itineraries == NULL)
        return;
    for (size_t i = 0; i < n_itineraries; i++)
    {
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
    for (size_t i = 0; i < n_raptor_routes; i++)
    {
        struct raptor_route *r = raptor_routes[i];
        if (r == NULL)
            continue;

        if (r->route_stops != NULL)
        {
            for (size_t j = 0; j < r->n_stops; j++)
            {
                struct raptor_route_stop *rrs = r->route_stops[j];
                if (rrs == NULL)
                    continue;
                free(rrs);
            }
        }
        free(r->route_stops);
        for (size_t j = 0; j < r->n_trips; j++)
        {
            free(r->stop_times[j]);
        }

        free(r->stop_times);
        free(r);
    }
    for (size_t i = 0; i < n_raptor_stops; i++)
    {
        struct raptor_stop *rs = raptor_stops[i];
        if (rs == NULL)
            continue;
        free(rs->routes_stops);
        free(rs->transfers);
        free(rs);
    }
    if (raptor_result != NULL)
    {
        if (raptor_result->parent_stops != NULL)
            free(raptor_result->parent_stops);
        if (raptor_result->parent_transfers != NULL)
            free(raptor_result->parent_transfers);
        if (raptor_result->parent_trips != NULL)
            free(raptor_result->parent_trips);
        if (raptor_result->tau_rounds)
        {
            for (size_t i = 0; i < raptor_result->total_rounds; i++)
            {
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
    if (raptor_result->tau_rounds)
    {
        for (size_t i = 0; i < raptor_result->total_rounds; i++)
        {
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

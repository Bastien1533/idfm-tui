#include "tui/ui/selector.h"
#include <notcurses/notcurses.h>

#include "data/gtfs.h"
#include "tui/worker/search.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct matching_item {
        int dist;
        struct ncselector_item *item;
};

int matching_itm_cmp(const void *s1, const void *s2)
{
        const struct matching_item *m1 =
                *(const struct matching_item *const *)s1;
        const struct matching_item *m2 =
                *(const struct matching_item *const *)s2;

        if (m1 == NULL && m2 == NULL)
                return 0;
        if (m1 == NULL)
                return 1;
        if (m2 == NULL)
                return -1;

        return (m1->dist == m2->dist) ? 0 : ((m1->dist < m2->dist) ? -1 : 1);
}

void free_matching_items(struct matching_item **items, size_t n_items,
                         bool full)
{
        if (items == NULL)
                return;
        if (full == true) {
                for (size_t i = 0; i < n_items; i++) {
                        struct matching_item *m_itm = items[i];
                        free_selector_item(m_itm->item);
                        free(m_itm);
                }
        } else {
                for (size_t i = 0; i < n_items; i++) {
                        struct matching_item *m_itm = items[i];
                        free(m_itm);
                }
        }
}

char *get_station_desc(struct stop *stop)
{
        if (stop == NULL)
                return NULL;
        if (stop->parent_station != NULL)
                stop = stop->parent_station;

        char *desc;
        asprintf(&desc, "%s", stop->stop_name);
        size_t len = strlen(desc);

        for (size_t i = 0; i < stop->n_assoc_routes; i++) {
                struct route *r = stop->assoc_routes[i];
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
        return desc;
}

struct search_res *get_search_stops(logic_task_stop_req *request,
                                    struct stop **stops, size_t n_stops)
{
        size_t max_n = (n_stops < 10) ? n_stops : 10;

        struct matching_item **matching_items =
                calloc(n_stops, sizeof(struct matching_item *));
        if (matching_items == NULL)
                return NULL;

        size_t match_n = 0;
        for (size_t i = 0; i < n_stops; i++) {
                struct stop *s = stops[i];
                if (s == NULL || s->stop_id == NULL ||
                    s->parent_station != NULL ||
                    strcasestr(s->stop_name, request->stop_name) == NULL)
                        continue;

                struct matching_item *match_item =
                        calloc(1, sizeof(struct matching_item));
                if (match_item == NULL)
                        continue;
                struct ncselector_item *item =
                        calloc(1, sizeof(struct ncselector_item));
                if (item == NULL) {
                        free(match_item);
                        continue;
                }

                item->desc = get_station_desc(stops[i]);
                item->option = strdup(stops[i]->stop_id);
                match_item->item = item;
                match_item->dist =
                        (int)(strcasestr(s->stop_name, request->stop_name) -
                              s->stop_name) *
                                1000 +
                        (int)(strlen(s->stop_name) -
                              strlen(request->stop_name));
                matching_items[i] = match_item;
                match_n++;
        }
        if (match_n > 0) {
                qsort(matching_items, n_stops, sizeof(struct matching_item *),
                      matching_itm_cmp);

                struct ncselector_item **items =
                        calloc(match_n, sizeof(struct ncselector_item *));
                if (items == NULL) {
                        free_matching_items(matching_items, n_stops, true);
                        return NULL;
                }
                size_t i = 0;
                size_t j = 0;
                while (i < match_n && j < max_n) {
                        if (matching_items[i] != NULL) {
                                items[i] = matching_items[i]->item;
                                j++;
                        }
                        i++;
                }
                free_matching_items(matching_items, i, false);
                free_matching_items(matching_items + i, match_n - i, true);
                free(matching_items);

                search_res *result = calloc(1, sizeof(search_res));
                if (result == NULL)
                        return NULL;

                result->items = items;
                result->n_res = j;
                return result;
        } else {
                search_res *result = calloc(1, sizeof(search_res));
                if (result == NULL)
                        return NULL;

                result->items = NULL;
                result->n_res = 0;
                return result;
        }
}

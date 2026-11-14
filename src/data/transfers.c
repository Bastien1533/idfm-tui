#include "data/transfers.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data/dataset.h"
#include "data/gtfs.h"

void insert_edge(struct node **vertices, size_t from_stop_idx,
                 struct stop *to_stop, struct pathway *path)
{
    struct node *head = vertices[from_stop_idx];
    // if (head == NULL) vert_count++;

    struct node *curr = head;

    bool dup = false;
    while (curr != NULL)
    {
        if (curr->stop->idx == to_stop->idx)
        {
            dup = true;
            break;
        }
        curr = curr->next;
    }
    if (dup == true)
        return;

    struct node *n = calloc(1, sizeof(struct node));
    if (n != NULL)
    {
        n->stop = to_stop;
        n->path = path;

        n->next = vertices[from_stop_idx];
        vertices[from_stop_idx] = n;
    }
}

struct pathway_graph *build_pathway_graph(struct pathway **pathways,
                                          size_t n_pathways, size_t n_stops)
{
    struct pathway_graph *graph = calloc(1, sizeof(struct pathway_graph));
    struct node **vertices = calloc(n_stops, sizeof(struct node *));
    for (size_t i = 0; i < n_pathways; i++)
    {
        struct pathway *path = pathways[i];
        if (path == NULL || path->from_stop == NULL || path->to_stop == NULL)
        {
            continue;
        }

        insert_edge(vertices, path->from_stop->idx, path->to_stop, path);
        if (path->is_bidirectional != 0)
        {
            insert_edge(vertices, path->to_stop->idx, path->from_stop, path);
        }
    }
    graph->vertices = vertices;
    graph->n_vert = n_stops;
    return graph;
}

size_t get_cost_djikstra(size_t x_idx, size_t y_idx,
                         struct pathway_graph *pathway_graph)
{
    if (x_idx == y_idx)
        return 0;

    if (pathway_graph == NULL || pathway_graph->vertices == NULL
        || pathway_graph->vertices[x_idx] == NULL)
    {
        return UINT_MAX;
    }
    struct node *curr = pathway_graph->vertices[x_idx];
    while (curr != NULL)
    {
        if (curr->stop->idx == y_idx)
        {
            return curr->path->traversal_time;
        }
        curr = curr->next;
    }
    return UINT_MAX;
}

struct pathway *get_pathway_cost_djisktra(size_t x_idx, size_t y_idx,
                                          struct pathway_graph *pathway_graph)
{
    if (x_idx == y_idx)
        return NULL;

    if (pathway_graph == NULL || pathway_graph->vertices == NULL
        || pathway_graph->vertices[x_idx] == NULL)
    {
        return NULL;
    }
    struct node *curr = pathway_graph->vertices[x_idx];
    while (curr != NULL)
    {
        if (curr->stop->idx == y_idx)
        {
            return curr->path;
        }
        curr = curr->next;
    }
    return NULL;
}

bool is_array_empty(bool *arr, size_t n_elems)
{
    for (size_t i = 0; i < n_elems; i++)
    {
        if (arr[i] == true)
            return false;
    }
    return true;
}

size_t get_min(bool *M, size_t n_elems, size_t *dist)
{
    size_t min = UINT_MAX;
    size_t min_idx = UINT_MAX;
    for (size_t i = 0; i < n_elems; i++)
    {
        if (M[i] == true && dist[i] < min)
        {
            min = dist[i];
            min_idx = i;
        }
    }
    return min_idx;
}

size_t out_deg(size_t x, struct pathway_graph *G)
{
    if (G == NULL || G->vertices == NULL || G->vertices[x] == NULL)
    {
        return 0;
    }
    size_t odeg = 0;

    struct node *curr = G->vertices[x];
    while (curr != NULL)
    {
        odeg++;
        curr = curr->next;
    }
    return odeg;
}

struct node *get_succ(size_t x, size_t succ_idx, struct pathway_graph *G)
{
    if (G == NULL || G->vertices == NULL || G->vertices[x] == NULL)
    {
        return NULL;
    }
    size_t i = 0;

    struct node *curr = G->vertices[x];
    while (curr != NULL && i < succ_idx)
    {
        i++;
        curr = curr->next;
    }
    if (i == succ_idx)
        return curr;
    return NULL;
}

void djikstra_get_detailled_transfer(struct transfer *transfer,
                                     struct pathway_graph *pathway_graph,
                                     size_t *dist, size_t *pred,
                                     struct pathway **pred_path)
{
    bool *M = calloc(pathway_graph->n_vert, sizeof(bool));
    for (size_t i = 0; i < pathway_graph->n_vert; i++)
    {
        M[i] = false;
    }

    for (size_t x = 0; x < pathway_graph->n_vert; x++)
    {
        dist[x] = get_cost_djikstra(transfer->from_stop->idx, x, pathway_graph);
        pred[x] = x;
        pred_path[x] = get_pathway_cost_djisktra(transfer->from_stop->idx, x,
                                                 pathway_graph);
        M[x] = true;
    }
    M[transfer->from_stop->idx] = false;

    size_t x;
    while (is_array_empty(M, pathway_graph->n_vert) == false)
    {
        x = get_min(M, pathway_graph->n_vert, dist);
        if (x == UINT_MAX || dist[x] == UINT_MAX)
        {
            free(M);
            return;
        }
        M[x] = false;

        for (struct node *succ = pathway_graph->vertices[x]; succ != NULL;
             succ = succ->next)
        {
            size_t y = succ->stop->idx;
            size_t w = succ->path->traversal_time;
            if (dist[x] + w < dist[y])
            {
                dist[y] = dist[x] + w;
                pred[y] = x;
                pred_path[y] = succ->path;
            }
        }
    }
    free(M);
}

void get_transfer_buckets(struct transfer **transfers, size_t n_transfers,
                          size_t n_stops, struct transfer ***buckets,
                          size_t *buckets_len)
{
    for (size_t i = 0; i < n_stops; i++)
    {
        buckets_len[i] = 0;
    }
    for (size_t i = 0; i < n_transfers; i++)
    {
        struct transfer *t = transfers[i];
        buckets[t->from_stop->idx] = realloc(
            buckets[t->from_stop->idx],
            (buckets_len[t->from_stop->idx] + 1) * sizeof(struct transfer *));
        buckets[t->from_stop->idx][buckets_len[t->from_stop->idx]] = t;
        buckets_len[t->from_stop->idx]++;
    }
}

void update_transfers_from_pathways(struct pathway **pathways,
                                    size_t n_pathways, struct stop **stops,
                                    size_t n_stops, struct transfer **transfers,
                                    size_t n_transfers,
                                    struct hash_map *transfers_map,
                                    struct hash_map *stop_map)
{
    struct pathway_graph *graph =
        build_pathway_graph(pathways, n_pathways, n_stops);

    struct transfer ***buckets = calloc(n_stops, sizeof(struct transfer **));
    size_t *buckets_len = calloc(n_stops, sizeof(size_t));
    get_transfer_buckets(transfers, n_transfers, n_stops, buckets, buckets_len);

    size_t n_cache = 0;

    size_t *dist_arr = NULL;
    size_t *pred_arr = NULL;
    struct pathway **pred_path_arr = NULL;
    dist_arr = calloc(graph->n_vert, sizeof(size_t));
    pred_arr = calloc(graph->n_vert, sizeof(size_t));
    pred_path_arr = calloc(graph->n_vert, sizeof(struct pathway *));

    for (size_t stop_idx = 0; stop_idx < n_stops; stop_idx++)
    {
        if (buckets_len[stop_idx] == 0 || graph->vertices[stop_idx] == NULL)
            continue;
        struct transfer *base_t = buckets[stop_idx][0];
        if (base_t == NULL || base_t->steps != NULL)
            continue;

        djikstra_get_detailled_transfer(base_t, graph, dist_arr, pred_arr,
                                        pred_path_arr);

        for (size_t i = 0; i < buckets_len[base_t->from_stop->idx]; i++)
        {
            struct transfer *t = buckets[base_t->from_stop->idx][i];
            if (t == NULL)
                continue;

            size_t idx = t->to_stop->idx;
            if (dist_arr[idx] == UINT_MAX)
            {
                // printf("pas de chemin\n");
            }
            else
            {
                size_t guard = 0;
                size_t total = 0;
                size_t src = t->from_stop->idx;
                bool first = true;
                while (++guard < graph->n_vert)
                {
                    if (idx >= n_stops)
                        break;
                    if (idx == src)
                    { /* source: no cost */
                        // printf("%s (%s)\n", stops[idx]->stop_name,
                        // stops[idx]->stop_id);
                        break;
                    }
                    size_t p = pred_arr[idx];
                    if (p == UINT_MAX || p >= n_stops)
                        p = src; /* safety */
                    size_t c = (p == idx) ? get_cost_djikstra(src, idx, graph)
                                          : get_cost_djikstra(p, idx, graph);
                    total += c;
                    // if (!first) printf(" -> ");
                    first = false;
                    // printf("%s (%s) (+%zu s) ", stops[idx]->stop_name,
                    // stops[idx]->stop_id, c);

                    t->steps = realloc(
                        t->steps, (t->n_steps + 1) * sizeof(struct pathway *));
                    t->steps[t->n_steps] = (p == idx)
                        ? get_pathway_cost_djisktra(src, idx, graph)
                        : pred_path_arr[idx];
                    t->n_steps++;

                    if (p == idx)
                    {
                        idx = src;
                        continue;
                    } /* anchor -> print src next */
                    idx = p;
                }
                if (t->min_transfer_time == 540 || t->min_transfer_time == 240)
                    t->min_transfer_time = total;
                // printf("Temps total: %zus vs %hus | %zu | N cached: %zu \n",
                // total,t->min_transfer_time, t->n_steps, n_cache);
            }
            // printf("-----------------\n");
        }
    }
    if (dist_arr != NULL)
        free(dist_arr);
    if (pred_arr != NULL)
        free(pred_arr);
    if (pred_path_arr != NULL)
        free(pred_path_arr);
}

void free_pathway_graph(struct node **pathway_graph, size_t n_stops)
{
    if (pathway_graph == NULL)
        return;
    for (size_t i = 0; i < n_stops; i++)
    {
        struct node *curr = pathway_graph[i];
        while (curr != NULL)
        {
            struct node *temp = curr->next;
            free(curr);
            curr = temp;
        }
    }
    free(pathway_graph);
}

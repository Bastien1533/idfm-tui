#ifndef TRANSFERS_H
#define TRANSFERS_H

#include "data/gtfs.h"
#include "utils/hashmap.h"

struct node {
        struct stop *stop;
        struct pathway *path;
        struct node *next;
};

struct pathway_graph {
        struct node **vertices;
        size_t n_vert;
};

void update_transfers_from_pathways(struct pathway **pathways,
                                    size_t n_pathways, struct stop **stops,
                                    size_t n_stops, struct transfer **transfers,
                                    size_t n_transfers,
                                    struct hash_map *transfers_map,
                                    struct hash_map *dataset_stop_map);

void free_pathway_graph(struct node **pathway_graph, size_t n_stops);

#endif

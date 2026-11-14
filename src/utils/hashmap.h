#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdbool.h>
#include <stdlib.h>

struct hash_map *init_hashmap(size_t capacity);
size_t hash_func(char *key);
void *hash_map_get(struct hash_map *hash_map, char *key);
char **hash_map_keys(struct hash_map *hash_map, size_t *count);
void **hash_map_items(struct hash_map *hash_map, size_t *count);

void *hash_map_insert(struct hash_map *hash_map, char *key, void *value);
void *hash_map_remove(struct hash_map *hash_map, char *key);
void free_hashmap(struct hash_map *hash_map);
void debug_hashmap(struct hash_map *hash_map);

struct hash_node
{
    char *key;
    void *value;
    struct hash_node *prev;
    struct hash_node *next;

    struct hash_node *ins_prev;
    struct hash_node *ins_next;
};

struct hash_map
{
    size_t capacity;
    size_t size;
    size_t (*hashfunc)(char *);
    struct hash_node **nodes;

    struct hash_node *ins_head;
    struct hash_node *ins_tail;
};

#endif

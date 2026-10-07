#include "utils/hashmap.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t hash_func(char *key)
{
        size_t hash = 0;
        char *c = key;
        for (c = key; *c != '\0'; c++) {
                hash = (hash * 31) + (unsigned char)*c;
        }

        return hash;
}

struct hash_map *init_hashmap(size_t capacity)
{
        if (capacity <= 0) {
                capacity = 64;
        }

        struct hash_map *hash_map = malloc(sizeof(struct hash_map));
        struct hash_node **nodes =
                calloc(1, capacity * sizeof(struct hash_node *));

        hash_map->capacity = capacity;
        hash_map->size = 0;
        hash_map->hashfunc = hash_func;
        hash_map->nodes = nodes;

        return hash_map;
}

void free_hashmap(struct hash_map *hash_map)
{
        if (hash_map == NULL || hash_map->hashfunc == NULL ||
            hash_map->nodes == NULL) {
                return;
        }
        for (size_t i = 0; i < hash_map->capacity; i++) {
                struct hash_node *curr = hash_map->nodes[i];
                if (curr != NULL) {
                        if (curr->next == NULL && curr->prev == NULL) {
                                free(curr->key);
                                free(curr);
                        } else {
                                while (curr != NULL && curr->next != NULL) {
                                        curr = curr->next;
                                }
                                while (curr != NULL && curr->prev != NULL) {
                                        curr = curr->prev;
                                        free(curr->next->key);
                                        free(curr->next);
                                }
                                free(hash_map->nodes[i]->key);
                                free(hash_map->nodes[i]);
                        }
                }
        }
        free(hash_map->nodes);
        free(hash_map);
}

void hash_map_resize(struct hash_map *hash_map, size_t new_capacity)
{
        struct hash_node **new_array =
                calloc(new_capacity, sizeof(struct hash_node *));
        for (size_t i = 0; i < hash_map->capacity; i++) {
                struct hash_node *curr = hash_map->nodes[i];
                while (curr != NULL) {
                        struct hash_node *next = curr->next;
                        size_t hash =
                                (hash_map->hashfunc)(curr->key) % new_capacity;

                        // We insert the new node at head of bucket
                        curr->next = new_array[hash];

                        // In the case the bucket wasnt empty before, we link the new node as prev
                        // in old bucket head.
                        if (new_array[hash] != NULL) {
                                new_array[hash]->prev = curr;
                        }
                        new_array[hash] = curr;

                        curr->prev = NULL;
                        curr = next;
                }
        }
        free(hash_map->nodes);
        hash_map->nodes = new_array;
        hash_map->capacity = new_capacity;
}

void *hash_map_insert(struct hash_map *hash_map, char *key, void *value)
{
        if (hash_map == NULL || key == NULL || value == NULL ||
            hash_map->hashfunc == NULL || hash_map->nodes == NULL) {
                return NULL;
        }

        size_t hash = (hash_map->hashfunc)(key) % hash_map->capacity;
        struct hash_node *curr = hash_map->nodes[hash];
        while (curr != NULL) {
                if (strcmp(curr->key, key) == 0) { // Duplicate check
                        void *old_value = curr->value;
                        curr->value = value;
                        return old_value;
                }
                curr = curr->next;
        }

        if (hash_map->size * 4 >
            hash_map->capacity * 3) // Equivalent to size/capacity > 0.75
        {
                hash_map_resize(hash_map, hash_map->capacity * 2);
                hash = (hash_map->hashfunc)(key) % hash_map->capacity;
        }

        struct hash_node *new_node = malloc(sizeof(struct hash_node));
        new_node->key = strdup(key);
        new_node->value = value;

        // We insert the new node at head of bucket
        new_node->next = hash_map->nodes[hash];

        // In the case the bucket wasnt empty before, we link the new node as prev in
        // old bucket head.
        if (hash_map->nodes[hash] != NULL) {
                hash_map->nodes[hash]->prev = new_node;
        }
        hash_map->nodes[hash] = new_node;
        hash_map->size++;
        return NULL;
}

void *hash_map_get(struct hash_map *hash_map, char *key)
{
        if (hash_map == NULL || key == NULL || hash_map->hashfunc == NULL ||
            hash_map->nodes == NULL) {
                return NULL;
        }
        size_t hash = (hash_map->hashfunc)(key) % hash_map->capacity;

        struct hash_node *curr = hash_map->nodes[hash];
        // Traverse bucket chain
        while (curr != NULL) {
                if (strcmp(curr->key, key) == 0) {
                        return curr->value; // Key found
                }
                curr = curr->next;
        }

        return NULL; // Key not found
}

void *hash_map_remove(struct hash_map *hash_map, char *key)
{
        if (hash_map == NULL || key == NULL || hash_map->hashfunc == NULL ||
            hash_map->nodes == NULL) {
                return NULL;
        }
        size_t hash = (hash_map->hashfunc)(key) % hash_map->capacity;

        struct hash_node *curr = hash_map->nodes[hash];
        // Traverse bucket chain
        while (curr != NULL) {
                if (strcmp(curr->key, key) == 0) {
                        void *old_value = curr->value;
                        if (curr->prev != NULL && curr->next == NULL) {
                                curr->prev->next = NULL;
                                free(curr->key);
                                free(curr);
                                hash_map->size--;
                                return old_value;
                        }
                        if (curr->prev != NULL && curr->next != NULL) {
                                curr->prev->next = curr->next;
                                curr->next->prev = curr->prev;
                                hash_map->size--;
                                free(curr->key);
                                free(curr);
                                return old_value;
                        }
                        if (curr->prev == NULL) {
                                hash_map->nodes[hash] = curr->next;
                                if (curr->next != NULL)
                                        curr->next->prev = NULL;
                                free(curr->key);
                                free(curr);
                                hash_map->size--;
                                return old_value;
                        }
                }
                curr = curr->next;
        }
        return NULL;
}

void debug_hashmap(struct hash_map *hash_map)
{
        size_t biggest_chain = 0;
        for (size_t b = 0; b < hash_map->capacity; b++) {
                if (hash_map->nodes[b] == NULL)
                        continue;
                size_t chain_len = 0;

                printf("|%zu| Hash: %zu  [%s]", b,
                       (hash_map->hashfunc)(hash_map->nodes[b]->key),
                       hash_map->nodes[b]->key);
                struct hash_node *curr = hash_map->nodes[b]->next;
                while (curr != NULL) {
                        struct hash_node *next = curr->next;
                        printf(" -> ");
                        printf("[%s]", curr->key);
                        curr = next;
                        chain_len++;
                }
                if (chain_len > biggest_chain) {
                        biggest_chain = chain_len;
                }
                printf("\n");
        }
        printf("Hash Map | Biggest Chain: %zu | Size: %zu | Capacity: %zu\n",
               biggest_chain, hash_map->size, hash_map->capacity);
}

#include <stdlib.h>
#include "tui/protocol.h"

void free_search_res(search_res *res)
{
        if (res && res->items != NULL) {
                for (size_t i = 0; i < res->n_res; i++) {
                        if ((void *)res->items[i] == NULL) {
                                continue;
                        }
                        free((void *)res->items[i]->desc);
                        free((void *)res->items[i]->option);
                        free(res->items[i]);
                }
                free(res->orig_items);
                free(res->items);
        }
        free(res);
}

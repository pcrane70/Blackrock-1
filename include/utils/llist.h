#ifndef LLIST_H
#define LLIST_H

#include <stdlib.h>

typedef struct Node {

    void *data;
    struct Node *next;

} Node;

typedef struct LList {

    size_t size;

    Node *start;
    Node *end;
    void (*destroy) (void *);

} LList;

#define llist_size(list) ((list)->size)
#define llist_start(list) ((list)->start)
#define llist_end(list) ((list)->end)
#define llist_data(node) ((node)->data)

extern LList *llist_init (void (*destroy) (void *));
extern void llist_destroy (LList *list);
extern void llist_insert_next (LList *list, Node *node, void *data);

#endif
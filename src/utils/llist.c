#include "utils/llist.h"

static Node *llist_node_create (void *data) {

    Node *new_node = (Node *) malloc (sizeof (Node));
    if (new_node) {
        new_node->data = data;
        new_node->next = NULL;
    }

    return new_node;

}

static void llist_node_destroy (Node *node, void (*destroy) (void *)) {

    if (node) {
        if (node->data) {
            if (destroy) destroy (node->data);
            else free (node->data);
        }

        free (node);
    }

}

LList *llist_init (void (*destroy) (void *)) {

    LList *new_list = (LList *) malloc (sizeof (LList));
    if (new_list) {
        new_list->size = 0;
        new_list->start = NULL;
        new_list->end = NULL;
        new_list->destroy = destroy;
    }

    return new_list;

}

void llist_destroy (LList *list) {

    if (list) {
        Node *ptr = list->start, *next;
        while (ptr) {
            next = ptr->next;
            llist_node_destroy (ptr, list->destroy);
            ptr = next;
        }

        free (list);
    }

}

void llist_insert_next (LList *list, Node *node, void *data) {

    Node *new_node = llist_node_create (data);
    if (new_node) {
        // insert at the start of the list
        if (!node) {
            if (llist_size (list) == 0) list->end = new_node;

            new_node->next = NULL;
            list->start = new_node;
        }

        // insert somewhere else
        else {
            if (!node->next) list->end = new_node;

            new_node->next = node->next;
            node->next = new_node;
        }

        list->size++;
    }

}

// FIXME: add llist_remove

// TODO: add a node get data
// TODO: add a list sort based on a comparator
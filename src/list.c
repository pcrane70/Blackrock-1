#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "list.h"

void initList (List *list, void (*destroy)(void *data)) {

    list->size = 0;
    list->destroy = destroy;
    list->start = NULL;
    list->end = NULL;

}

bool removeAfter (List *list, ListElement *element, void **data) {

    ListElement *old;

    if (LIST_SIZE (list) == 0) return false;

    if (element == NULL) {
        *data = list->start->data;
        old = list->start;
        list->start = list->start->next;
    }

    else {
        if (element->next == NULL) return false;

        *data = element->next->data;
        old = element->next;
        element->next = element->next->next;

        if (element->next == NULL) list->end = element;
    }

    free (old);
    list->size--;

    return true;

}

void destroyList (List *list) {

    void *data;

    while (LIST_SIZE (list) > 0) 
        if (removeAfter (list, NULL, (void **) data) && list->destroy != NULL)
            list->destroy (data);

    memset (list, 0, sizeof (List));

}

bool insertAfter (List *list, ListElement *element, void **data) {

    ListElement *new;
    if ((new = (ListElement *) malloc (sizeof (ListElement))) == NULL) 
        return false;

    new->data = (void *) data;

    if (element == NULL) {
        if (LIST_SIZE (list) == 0) list->end = new;

        new->next = list->start;
        list->start = new;
    }

    else {
        if (element->next == NULL) list->end = new;

        new->next = element->next;
        element->next = new;
    }

    list->size++;

    return true;

}
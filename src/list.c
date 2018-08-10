#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "list.h"

List *initList (void (*destroy)(void *data)) {

    List *list = (List *) malloc (sizeof (List));

    // TODO: how to handle a failed allocation
    if (list != NULL) {
        list->size = 0;
        list->destroy = destroy;
        list->start = NULL;
        list->end = NULL;
    }

    return list;

}

void *removeElement (List *list, ListElement *element) {

    ListElement *old;
    void *data = NULL;

    if (LIST_SIZE (list) == 0) return false;

    if (element == NULL) {
        data = list->start->data;
        old = list->start;
        list->start = list->start->next;
        list->start->prev = NULL;
    }

    else {
        data = element->next->data;
        old = element;

        ListElement *prevElement = element->prev;
        ListElement *nextElement = element->next;

        if (prevElement != NULL && nextElement != NULL) {
            prevElement->next = nextElement;
            nextElement->prev = prevElement;
        }

        else {
            // we are at the start of the list
            if (prevElement == NULL) {
                if (nextElement != NULL) nextElement->prev = NULL;
                list->start = nextElement;
            }

            // we are at the end of the list
            if (nextElement == NULL) {
                if (prevElement != NULL) nextElement->next = NULL;
                list->end = prevElement;
            }
        }
    }

    free (old);
    list->size--;

    return data;

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
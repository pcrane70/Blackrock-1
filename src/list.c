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

    if (LIST_SIZE (list) == 0) return NULL;

    if (element == NULL) {
        data = list->start->data;
        old = list->start;
        list->start = list->start->next;
        if (list->start != NULL) list->start->prev = NULL;
    }

    else {
        data = element->data;
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
                if (prevElement != NULL) prevElement->next = NULL;
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

    while (LIST_SIZE (list) > 0) {
        data = removeElement (list, NULL);
        if (data != NULL && list->destroy != NULL) list->destroy (data);
    }

    free (list);

}

bool insertAfter (List *list, ListElement *element, void *data) {

    ListElement *new;
    if ((new = (ListElement *) malloc (sizeof (ListElement))) == NULL) 
        return false;

    new->data = (void *) data;

    if (element == NULL) {
        if (LIST_SIZE (list) == 0) list->end = new;
        else list->start->prev = new;
       
       new->next = list->start;
       new->prev = NULL;
       list->start = new;
    }

    else {
        if (element->next == NULL) list->end = new;

        new->next = element->next;
        new->prev = element;
        element->next = new;
    }

    list->size++;

    return true;

}
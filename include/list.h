#ifndef LIST_H
#define LIST_H

typedef struct ListElement {

    void *data;
    struct ListElement *prev;
    struct ListElement *next;

} ListElement;

typedef struct List {

    unsigned int size;

    void (*destroy)(void *data);

    ListElement *start;
    ListElement *end;

} List;

#define LIST_SIZE(list) ((list)->size)

#define LIST_START(list) ((list)->start)
#define LIST_END(list) ((list)->end)

#define LIST_DATA(element) ((element)->data)
#define LIST_NEXT(element) ((element)->next)


extern List *initList (void (*destroy)(void *data));
extern void *removeElement (List *list, ListElement *element);
extern void destroyList (List *);
extern bool insertAfter (List *list, ListElement *element, void **data);

#endif
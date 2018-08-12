// Implementation of a simple object pool system using a stack

// 08/08/2018 --> for now we only need one global pool for the GameObjects like monsters and various items
// the map is handled differntly using an array.

#include <stdlib.h>
#include <stdio.h>

#include "objectPool.h"

// 11/08/2018 -- object pool will be on hold for now...


// TODO: add the avility to init a pool with some members
Pool *initPool (void) {

    Pool *pool = (Pool *) malloc (sizeof (Pool));

    if (pool != NULL) {
        pool->size = 0;
        pool->top = NULL;
    }

    return pool;

}

void push (Pool *pool, void *data) {

    PoolMember *new = (PoolMember *) malloc (sizeof (PoolMember));
    new->data = data;

    if (POOL_SIZE (pool) == 0) new->next = NULL;
    else new->next = pool->top;

    pool->top = new;

}

void *pop (Pool *pool) {

    void *data;
    if (POOL_SIZE (pool) == 0) {

    }

}

// GameObject *popGO (GameObject **top) {

//     GameObject *ptr = *top;
//     // TODO: better error handling here!!
//     if (top == NULL) fprintf (stderr, "Stack underflow!!\n\n");
//     else top = &ptr->next;

//     return ptr;

// }


// GameObject *clearPool (GameObject *top) {

//     GameObject *ptr, *temp;
//     if (top != NULL) {
//         ptr = top;
//         while (ptr != NULL) {
//             temp = top;
//             top = top->next;
//             free (temp);
//             ptr = top;
//         }
//     }

//     return NULL;

// }

// Implementation of a simple object pool system using a stack

// 08/08/2018 --> for now we only need one global pool for the GameObjects like monsters and various items
// the map is handled differntly using an array.

#include "game.h"

GameObject *popGO (GameObject **top) {

    GameObject *ptr = *top;
    // TODO: better error handling here!!
    if (top == NULL) fprintf (stderr, "Stack underflow!!\n\n");
    else top = &ptr->next;

    return ptr;

}

GameObject *pushGO (GameObject *top, GameObject *go) {

    if (top == NULL) {
        go->next = NULL;
        top = go;
    }

    else {
        go->next = top;
        top = go;
    }

    return top;

}

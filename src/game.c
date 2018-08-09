/*** GAME MANAGER ***/

// 03/08/2018 -- 18:18
// For now this will be like our Game Controller, well kind of
// lets see how the game scales and the we will decide what to do

#include <assert.h>

#include "blackrock.h"
#include "game.h"
#include "objectPool.h"

/*** WORLD STATE ***/


/*** Game Object Management **/

// 08/08/2018 --> we now handle some GameObjects with a llist and a Pool;
// the map is managed using an array

// Also note that we still don't know how do we want the levels to regenerate when we die
// - do we want a new random level??
// or the same one and only regenerate the levels when we start a new session??

// 08/08/2018 -- 22:14
// we start the program with no objects in the pool
// TODO: maybe later we will want to have some in memory
static unsigned int inactive = 0;

// reference to the start of the pool
static GameObject *pool = NULL;

// TODO: how do we want to solve the problem of the first GO??

GameObject *createGOList (GameObject *first, GameObject *data) {

    GameObject *go, *ptr;

    go = (GameObject *) malloc (sizeof (GameObject));

    // FIXME: error handling
    if (go == NULL) return NULL;

    // FIXME: how to assign a unique id to each GO?
    // go->id = data->id;  

    go->x = data->x;
    go->y = data->y;
    go->glyph = data->glyph;
    go->fgColor = data->fgColor;
    go->bgColor = data->bgColor;
    go->blocksMovement = data->blocksMovement;
    go->blocksSight = data->blocksSight;

    if (first == NULL) {
        go->next = NULL;
        first = go;
    }

    else {
        ptr = first;
        while (ptr->next != NULL)
            ptr = ptr->next;

        ptr->next = go;
        go->next = NULL;
    }

    return go;

}

void createGO (GameObject *data) {

    GameObject *ptr = pool;
    GameObject *go = NULL;

    // if our object pool is empty, we create a new object
    if (inactive == 0) go = (GameObject *) malloc (sizeof (GameObject));
    else {
        // grab a GO from our Pool...
        go = popGO (&pool);
        inactive--;
    }

    // FIXME: how to assign a unique id to each GO?
        // go->id = data->id;

    go->x = data->x;
    go->y = data->y;
    go->glyph = data->glyph;
    go->fgColor = data->fgColor;
    go->bgColor = data->bgColor;
    go->blocksMovement = data->blocksMovement;
    go->blocksSight = data->blocksSight;

    // and add it directly to the end of the GO list
    while (ptr->next != NULL) ptr = ptr->next;

    ptr->next = go;

}

// This calls the Object pooling to deactive the go and have it in memory 
// to reuse it when we need it
void destroyGO (GameObject *go) {

    pushGO (pool, go);
    inactive++;

}

GameObject *deleteGOList (GameObject *first) {

    GameObject *ptr, *temp;
    if (first != NULL) {
        ptr = first;
        while (ptr != NULL) {
            temp = first;
            first = first->next;
            free (temp);
            ptr = first;
        }
    }

    return NULL;

}

GameObject *cleanGameObjects (GameObject *first) {

    // cleanup the pool
    pool = clearPool (pool);
    if (pool == NULL) fprintf (stdout, "\nPool has been cleared.\n");

    return deleteGOList (first);

}


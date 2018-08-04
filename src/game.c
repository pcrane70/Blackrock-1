// 03/08/2018 -- 18:18
// For now this will be like our Game Controller, well kind of
// lets see how the game scales and the we will decide what to do

#include <assert.h>

#include "blackrock.h"
#include "game.h"

// World State


/*** Game Object Management **/

// TODO: is it better to have an array or a list??
# define MAX_GO     1000

global GameObject gameObjects[MAX_GO];

// TODO: do we want this?? is there a better way around??
global Position positionComps[MAX_GO];


// as of 03/08/2018 we identify a free space in gameObjects array if it has an id = 0
GameObject *createGameObject () {

    // Get an available object space (memory)
    GameObject *go = NULL;
    for (u32 i = 1; i < MAX_GO; i++) {
        if (gameObjects[i].id == 0) {
            // Set up the new GameObject??
            go = &gameObjects[i];
            go->id = i;
            break;
        }
    }

    // are we returning a valid GO??
    assert (go != NULL);

    // TODO: do we want this here or in destriyObject??
    // cleanning up our components in the obj --- 04/08/2018
    for (u32 i = 0; i < MAX_COMP_COUNT; i++) 
        go->components[i] = NULL;

    return go;

}

void addComponentToGO (GameObject *obj, GameComponent comp, void *compData) {

    // make sure that we have a valid GO
    assert (obj->id != 0);

    switch (comp) {
        case POSITION: {
            Position *pos = &positionComps[obj->id];
            Position *posData = (Position *) compData;
            pos->objectId = obj->id;
            pos->x = posData->x;
            pos->y = posData->y;

            obj->components[comp] = &positionComps [obj->id];
            break; }

        default: fprintf (stderr, "Unknown component!\n"); break;
    }

}

void *getComponent (GameObject *obj, GameComponent comp) {

    return obj->components[comp];

}

// TODO: maybe in the future we will want to have a memory management system
// like an object pool... assuming that all of our objects have the same base!!

// as of 03/08/2018 we identify a free space in gameObjects array if it has an id = 0
void destroyGO (GameObject *obj) {

    // cleanning up the components in their arrays
    positionComps[obj->id].objectId = 0;

    obj->id = 0;

}
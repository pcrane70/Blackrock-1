/*** GAME MANAGER ***/

// 03/08/2018 -- 18:18
// For now this will be like our Game Controller, well kind of
// lets see how the game scales and the we will decide what to do

#include <assert.h>

#include "blackrock.h"
#include "game.h"

#include "list.h"
#include "objectPool.h"

/*** WORLD STATE ***/

void initWorld (void) {

    gameObjects = initList (free);
    positions = initList (free);
    graphics = initList (free);
    physics = initList (free);

    // TODO: what other things do we want to init here?

}


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

// 11/08/2018 -- we will assign a new id to each new GO starting at 0
static unsigned int id = 0;

// Adds an object to our GO list
GameObject *createGO () {

    // TODO:

}

void addComponent (GameObject *go, GameComponent type, void *data) {

    // TODO: check for a valid GO
    // TODO: how do we handle if the data is NULL?

    // TODO: object pooling for components

    switch (type) {
        case POSITION: {
            if (getComponent (go, type) != NULL) return;
            Position *newPos = (Position *) malloc (sizeof (Position));
            Position *posData = (Position *) data;
            newPos->objectId = go->id;
            newPos->x = posData->x;
            newPos->y = posData->y;
            newPos->layer = posData->layer;
            insertAfter (positions, NULL, newPos);
        }
        case GRAPHICS: {
            if (getComponent (go, type) != NULL) return;
            Graphics *newGraphics = (Graphics *) malloc (sizeof (Graphics));
            Graphics *graphicsData = (Graphics *) data;
            newGraphics->objectId = go->id;
            newGraphics->glyph = graphicsData->glyph;
            newGraphics->fgColor = graphicsData->fgColor;
            newGraphics->bgColor = graphicsData->bgColor;
            insertAfter (graphics, NULL, newGraphics);
        }
        case PHYSICS: {
            if (getComponent (go, type) != NULL) return;
            Physics *newPhys = (Physics *) malloc (sizeof (Physics));
            Physics *physData = (Physics *) data;
            newPhys->objectId = go->id;
            newPhys->blocksSight = physData->blocksSight;
            newPhys->blocksMovement = physData->blocksMovement;
            insertAfter (graphics, NULL, newPhys);
        }

        // We have an invalid GameComponent type
        // TODO: how to handle this??
        default: return;
    }

}

void *getComponent (GameObject *go, GameComponent type) {

    // TODO: check if this works properlly
    void *retVal = go->components[type];
    if (retVal == NULL) return NULL;
    else return retVal;

}

// FIXME:
// This calls the Object pooling to deactive the go and have it in memory 
// to reuse it when we need it
void destroyGO (GameObject *go) {

    pushGO (pool, go);
    inactive++;

}

// FIXME:
GameObject *cleanGameObjects (GameObject *first) {

    // cleanup the pool
    pool = clearPool (pool);
    if (pool == NULL) fprintf (stdout, "\nPool has been cleared.\n");

    return deleteGOList (first);

}


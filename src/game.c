/*** GAME MANAGER ***/

// 03/08/2018 -- 18:18
// For now this will be like our Game Controller, well kind of
// lets see how the game scales and the we will decide what to do

#include <assert.h>

#include "blackrock.h"
#include "game.h"
#include "map.h"

#include "list.h"
#include "objectPool.h"

/*** WORLD STATE ***/

List *gameObjects = NULL;
List *positions = NULL;
List *graphics = NULL;
List *physics = NULL;

Pool *goPool = NULL;
Pool *posPool = NULL;
Pool *graphicsPool = NULL;
Pool *physPool = NULL;

GameObject *player = NULL;

// Inits the wolrd, this will be all the 'physical part' that takes place in a world
void initWorld (void) {

    gameObjects = initList (free);
    positions = initList (free);
    graphics = initList (free);
    physics = initList (free);

    // init our pools
    goPool = initPool ();
    posPool = initPool ();
    graphicsPool = initPool ();
    physPool = initPool ();

    // TODO: what other things do we want to init here?

}

// Inits the global state of the game
// Inits all the data and structures for an initial game
// This should be called only once when we init run the app
void initGame (void) {

    initWorld ();

    GameObject *initPlayer (void);
    player = initPlayer ();
    // TODO: player name

    // TODO:
    // aftwe we have initialize our structures and allocated the memory,
    // we will want to load the in game menu (tavern)

    // FIXME: as of 12/08/2018 -- 19:43 -- we don't have the tavern ready, so we will go
    // straigth into the dungeon...

    // TODO: make sure that we have cleared the last level data
    // clear gameObjects and properly handle memory 

    // TODO: this can be a good place to check if we have a save file of a map and load thhat from disk

    // currentLevel = (Level *) malloc (sizeof (Level));
    // currentLevel->levelNum = 1;
    // currentLevel->mapCells = (bool **) calloc (MAP_WIDTH, sizeof (bool *));
    // for (short unsigned int i = 0; i < MAP_WIDTH; i++)
    //     currentLevel->mapCells[i] = (bool *) calloc (MAP_HEIGHT, sizeof (bool));

    // generate a random world froms scratch
    // TODO: maybe later we want to specify some parameters based on difficulty?
    // or based on the type of terrain that we want to generate.. we don't want to have the same algorithms
    // to generate rooms and for generating caves or open fiels

    // after we have allocated the new level, generate the map
    // this is used to render the walls to the screen... but maybe it is not a perfect system
    // initMap (currentLevel->mapCells);

    // TODO: after the map has been init, place all the objects, NPCs and enemies, etc


    // finally, we have a map full with monsters, so we can place the player and we are done 
    // Point playerSpawnPos = getFreeSpot (currentLevel->mapCells);
    // Position *playerPos = (Position *) getComponent (player, POSITION);
    // playerPos->x = (u8) playerSpawnPos.x;
    // playerPos->y = (u8) playerSpawnPos.y;

     fprintf (stdout, "Done initializing game!\n");

}



/*** Game Object Management **/

// because the player is an special GO, we want to initialize him differently
GameObject *initPlayer (void) {

    GameObject *go = (GameObject *) malloc (sizeof (GameObject));
    go->id = 0;

    for (short unsigned int i = 0; i < COMP_COUNT; i++)
        go->components[i] = NULL;

    // This is just a placeholder until it spawns in the world
    Position pos = { .x = 0, .y = 0, .layer = TOP_LAYER };
    addComponent (go, POSITION, &pos);

    Graphics g = { 0, '@', 0xFFFFFFFF, 0x000000FF };
    addComponent (go, GRAPHICS, &g);

    Physics phys = { 0, true, true };
    addComponent (go, PHYSICS, &phys);

    return go;

}


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

// 11/08/2018 -- we will assign a new id to each new GO starting at 1
// id = 0 is the player 
static unsigned int newId = 1;


GameObject *createGO () {

    GameObject *go = NULL;

    // first check if there is an available one in the pool
    if (POOL_SIZE (goPool) > 0) go = pop (goPool);
    else go = (GameObject *) malloc (sizeof (GameObject));

    if (go != NULL) {
        go->id = newId;
        newId++;
        for (short unsigned int i = 0; i < COMP_COUNT; i++) go->components[i] = NULL;
        insertAfter (gameObjects, LIST_END (gameObjects), go);
    }
    
    return go;

}

void addComponent (GameObject *go, GameComponent type, void *data) {

    // check for a valid GO
    if ((go != NULL) && (isInList (gameObjects, go) != false)) return;

    // if data is NULL for any reason, just don't do anything
    if (data == NULL) return;

    switch (type) {
        case POSITION: {
            if (getComponent (go, type) != NULL) return;

            Position *newPos = NULL;

            if ((POOL_SIZE (posPool) > 0)) newPos = pop (posPool);
            else newPos = (Position *) malloc (sizeof (Position));

            Position *posData = (Position *) data;
            newPos->objectId = go->id;
            newPos->x = posData->x;
            newPos->y = posData->y;
            newPos->layer = posData->layer;

            go->components[type] = newPos;
            insertAfter (positions, NULL, newPos);
        }
        case GRAPHICS: {
            if (getComponent (go, type) != NULL) return;

            Graphics *newGraphics = NULL;

            if ((POOL_SIZE (graphicsPool) > 0)) newGraphics = pop (graphicsPool);
            else newGraphics = (Graphics *) malloc (sizeof (Graphics));

            Graphics *graphicsData = (Graphics *) data;
            newGraphics->objectId = go->id;
            newGraphics->glyph = graphicsData->glyph;
            newGraphics->fgColor = graphicsData->fgColor;
            newGraphics->bgColor = graphicsData->bgColor;

            go->components[type] = newGraphics;
            insertAfter (graphics, NULL, newGraphics);
        }
        case PHYSICS: {
            if (getComponent (go, type) != NULL) return;

            Physics *newPhys = NULL;

            if ((POOL_SIZE (physPool) > 0)) newPhys = pop (physPool);
            else newPhys = (Physics *) malloc (sizeof (Physics));

            Physics *physData = (Physics *) data;
            newPhys->objectId = go->id;
            newPhys->blocksSight = physData->blocksSight;
            newPhys->blocksMovement = physData->blocksMovement;

            go->components[type] = newPhys;
            insertAfter (graphics, NULL, newPhys);
        }

        // We have an invalid GameComponent type, so don't do anything
        default: break;
    }

}


void updateComponent (GameObject *go, GameComponent type, void *data) {

    // check for a valid GO
    if ((go != NULL) && (isInList (gameObjects, go) != false)) return;

    // if data is NULL for any reason, just don't do anything
    if (data == NULL) return;

    switch (type) {
        case POSITION: {
            Position *posComp = (Position *) getComponent (go, type);
            if (posComp == NULL) return;
            Position *posData = (Position *) data;
            posComp->x = posData->x;
            posComp->y = posData->y;
            posComp->layer = posData->layer;
        }
        case GRAPHICS: {
            Graphics *graphicsComp = (Graphics *) getComponent (go, type);
            if (graphicsComp == NULL) return;
            Graphics *graphicsData = (Graphics *) data;
            graphicsComp->glyph = graphicsData->glyph;
            graphicsComp->fgColor = graphicsData->fgColor;
            graphicsComp->bgColor = graphicsComp->bgColor;
        }
        case PHYSICS: {
            Physics *physComp = (Physics *) getComponent (go, type);
            if (physComp == NULL) return;
            Physics *physData = (Physics *) data;
            physComp->blocksMovement = physData->blocksMovement;
            physComp->blocksSight = physData->blocksSight;
        }

        // We have an invalid GameComponent type, so don't do anything
        default: break;
    }

}

void *getComponent (GameObject *go, GameComponent type) {

    // TODO: check if this works properlly
    void *retVal = go->components[type];
    if (retVal == NULL) return NULL;
    else return retVal;

}

// This calls the Object pooling to deactive the go and have it in memory 
// to reuse it when we need it
void destroyGO (GameObject *go) {

    ListElement *e = NULL;
    void *removed = NULL;

    // get the game object to remove and then send it to the its pool
    if ((e = getListElement (gameObjects, go)) != NULL) {
        removed = removeElement (gameObjects, e);
        // clean the go components
        for (short unsigned int i = 0; i < COMP_COUNT; i++)
            go->components[i] = NULL;

        // send to the GO pool
        push (goPool, removed);
    }

    if ((e = getListElement (positions, go->components[POSITION])) != NULL) {
        removed = removeElement (positions, e);
        push (posPool, removed);
    }

    if ((e = getListElement (graphics, go->components[GRAPHICS])) != NULL) {
        removed = removeElement (graphics, e);
        push (graphicsPool, removed);
    }

    if ((e = getListElement (physics, go->components[PHYSICS])) != NULL) {
        removed = removeElement (physics, e);
        push (physPool, removed);
    }

}

void cleanUpGame (void) {

    // clean up our lists
    destroyList (gameObjects);
    destroyList (positions);
    destroyList (graphics);
    destroyList (physics);

    // cleanup the pools
    clearPool (goPool);
    clearPool (posPool);
    clearPool (graphicsPool);
    clearPool (physPool);

}


/*** LEVEL MANAGER ***/




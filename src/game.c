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

#include "utils/myUtils.h"


/*** WORLD STATE ***/

List *gameObjects = NULL;
List *positions = NULL;
List *graphics = NULL;
List *physics = NULL;
List *movement = NULL;
List *combat = NULL;
List *items = NULL;

Pool *goPool = NULL;
Pool *posPool = NULL;
Pool *graphicsPool = NULL;
Pool *physPool = NULL;
Pool *movePool = NULL;
Pool *combatPool = NULL;
Pool *itemsPool = NULL;

// PLayer
GameObject *player = NULL;
bool playerTookTurn = false;
// This is the player's inventory
static List *inventory = NULL;
static i32 maxWeight = 20;

// FOV
static u32 fovMap[MAP_WIDTH][MAP_HEIGHT];
static bool recalculateFov = false;

// Inits the wolrd, this will be all the 'physical part' that takes place in a world
void initWorld (void) {

    gameObjects = initList (free);
    positions = initList (free);
    graphics = initList (free);
    physics = initList (free);
    movement = initList (free);
    combat = initList (free);
    items = initList (free);

    // init our pools
    goPool = initPool ();
    posPool = initPool ();
    graphicsPool = initPool ();
    physPool = initPool ();
    movePool = initPool ();
    combatPool = initPool ();
    itemsPool = initPool ();

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

    currentLevel = (Level *) malloc (sizeof (Level));
    currentLevel->levelNum = 1;
    currentLevel->mapCells = (bool **) calloc (MAP_WIDTH, sizeof (bool *));
    for (short unsigned int i = 0; i < MAP_WIDTH; i++)
        currentLevel->mapCells[i] = (bool *) calloc (MAP_HEIGHT, sizeof (bool));

    // currentLevel->walls = (Wall **) calloc (MAP_WIDTH, sizeof (Wall *));
    // for (short unsigned int i = 0; i < MAP_WIDTH; i++)
    //     currentLevel->walls[i] = (Wall *) calloc (MAP_HEIGHT, sizeof (Wall));

    // generate a random world froms scratch
    // TODO: maybe later we want to specify some parameters based on difficulty?
    // or based on the type of terrain that we want to generate.. we don't want to have the same algorithms
    // to generate rooms and for generating caves or open fiels

    // after we have allocated the new level, generate the map
    // this is used to render the walls to the screen... but maybe it is not a perfect system
    initMap (currentLevel->mapCells);

    // TODO: after the map has been init, place all the objects, NPCs and enemies, etc

    fprintf (stdout, "Creating monsters...\n");
    // 14/08/2018 -- 23:02 -- spawn some monsters to test how they behave
    void createMonster (GameObject *);
    for (short unsigned int i = 0; i < 2; i++) {
        GameObject *monster = createGO ();
        createMonster (monster);
        // spawn in a random position
        Point monsterSpawnPos = getFreeSpot (currentLevel->mapCells);
        Position *monsterPos = (Position *) getComponent (monster, POSITION);
        monsterPos->x = (u8) monsterSpawnPos.x;
        monsterPos->y = (u8) monsterSpawnPos.y;
        // TODO: mark the spawnPos as filled
        fprintf (stdout, "Created a new monster!\n");
    }

    // 15/08/2018 -- 18:09 -- lets sprinckle some items trough the level and see how they behave
    void createItem (char *name, u8 xPos, u8 yPos, u8 layer,
     asciiChar glyph, u32 fgColor, i32 quantity, i32 weight, i32 lifetime);
    for (short unsigned int i = 0; i < 2; i++) {
        Point spawnPos = getFreeSpot (currentLevel->mapCells);
        createItem ("Test Item", spawnPos.x, spawnPos.y, MID_LAYER, 'I', 0xFFFFFFFF, 1, 1, 20);
    }   

    // finally, we have a map full with monsters, so we can place the player and we are done 
    fprintf (stdout, "Spawning the player...\n");
    Point playerSpawnPos = getFreeSpot (currentLevel->mapCells);
    Position *playerPos = (Position *) getComponent (player, POSITION);
    playerPos->x = (u8) playerSpawnPos.x;
    playerPos->y = (u8) playerSpawnPos.y;

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
    // FIXME: isInList not working properly...
    // if ((go != NULL) && (isInList (gameObjects, go) != false)) return;

    if (go == NULL) return;

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
        } break;
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
        } break;
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
        } break;
        case MOVEMENT: {
            if (getComponent (go, type) != NULL) return;

            Movement *newMove = NULL;

            if ((POOL_SIZE (movePool) > 0)) newMove = pop (movePool);
            else newMove = (Movement *) malloc (sizeof (Movement));

            Movement *movData = (Movement *) data;
            newMove->objectId = go->id;
            newMove->speed = movData->speed;
            newMove->frecuency = movData->frecuency;
            newMove->ticksUntilNextMov = movData->ticksUntilNextMov;
            newMove->chasingPlayer = movData->chasingPlayer;
            newMove->turnsSincePlayerSeen = movData->turnsSincePlayerSeen;

            go->components[type] = newMove;
            insertAfter (movement, NULL, newMove);
        } break;
        case ITEM: {
            if (getComponent (go, type) != NULL) return;

            Item *newItem = NULL;

            if ((POOL_SIZE (itemsPool) > 0)) newItem = pop (itemsPool);
            else newItem = (Item *) malloc (sizeof (Item));

            Item *itemData = (Item *) data;
            newItem->objectId = go->id;
            newItem->slot = itemData->slot;
            newItem->weight = itemData->weight;
            newItem->quantity = itemData->quantity;
            newItem->lifetime = itemData->lifetime;
            newItem->isEquipped = false;

            go->components[type] = newItem;
            insertAfter (items, NULL, newItem);
        } break;

        // We have an invalid GameComponent type, so don't do anything
        default: break;
    }

}


void updateComponent (GameObject *go, GameComponent type, void *data) {

    // check for a valid GO
    // FIXME: isInList not working properly...
    // if ((go != NULL) && (isInList (gameObjects, go) != false)) return;

    if (go == NULL) return;

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
        } break;
        case GRAPHICS: {
            Graphics *graphicsComp = (Graphics *) getComponent (go, type);
            if (graphicsComp == NULL) return;
            Graphics *graphicsData = (Graphics *) data;
            graphicsComp->glyph = graphicsData->glyph;
            graphicsComp->fgColor = graphicsData->fgColor;
            graphicsComp->bgColor = graphicsComp->bgColor;
        } break;
        case PHYSICS: {
            Physics *physComp = (Physics *) getComponent (go, type);
            if (physComp == NULL) return;
            Physics *physData = (Physics *) data;
            physComp->blocksMovement = physData->blocksMovement;
            physComp->blocksSight = physData->blocksSight;
        } break;
        case MOVEMENT: {
            Movement *movComp = (Movement *) getComponent (go, type);
            if (movComp == NULL) return;
            Movement *movData = (Movement *) data;
            movComp->speed = movData->speed;
            movComp->frecuency = movData->frecuency;
            movComp->ticksUntilNextMov = movData->ticksUntilNextMov;
            movComp->chasingPlayer = movData->chasingPlayer;
            movComp->turnsSincePlayerSeen = movData->turnsSincePlayerSeen;
        } break;
        case ITEM: {
            Item *itemComp = (Item *) getComponent (go, type);
            if (itemComp == NULL) return;
            Item *itemData = (Item *) data;
            itemComp->objectId = go->id;
            itemComp->slot = itemData->slot;
            itemComp->weight = itemData->weight;
            itemComp->quantity = itemData->quantity;
            itemComp->lifetime = itemData->lifetime;
            itemComp->isEquipped = itemData->isEquipped;
        } break;

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

    if ((e = getListElement (movement, go->components[MOVEMENT])) != NULL) {
        removed = removeElement (movement, e);
        push (movePool, removed);
    }

}

void cleanUpGame (void) {

    // clean up our lists
    destroyList (gameObjects);
    destroyList (positions);
    destroyList (graphics);
    destroyList (physics);
    destroyList (movement);
    destroyList (items);

    // cleanup the pools
    clearPool (goPool);
    clearPool (posPool);
    clearPool (graphicsPool);
    clearPool (physPool);
    clearPool (movePool);
    clearPool (itemsPool);

}


/*** LEVEL MANAGER ***/

// We are testing how do we want to create items in the level
// TODO: how do we want to manage lifetim - durability?
// TODO: later we will need to add some combat parametres
// FIXME: how do we want to manage the name?
void createItem (char *name, u8 xPos, u8 yPos, u8 layer,
     asciiChar glyph, u32 fgColor, i32 quantity, i32 weight, i32 lifetime) {

    GameObject *item = createGO ();

    Position pos = { .x = xPos, .y = yPos, .layer = layer };
    addComponent (item, POSITION, &pos);
    Graphics g = { .glyph = glyph, .fgColor = fgColor, .bgColor = 0x000000FF };
    addComponent (item, GRAPHICS, &g);
    Physics phys = { .blocksMovement = false, .blocksSight = false };
    addComponent (item, PHYSICS, &phys);
    Item i = { .quantity = quantity, .weight = weight, .isEquipped = false, .lifetime = lifetime };
    addComponent (item, ITEM, &i);
        
}

// TODO:
/*** INVENTORY ***/

// we want to press I and open a new window that shows our inventory
// TODO: maybe if we press the letter c, we can get a window showing our current equipment


/*** MOVEMENT ***/

bool isWall (u32 x, u32 y) { return (currentLevel->mapCells[x][y]); }

bool canMove (Position pos) {

    bool move = true;

    // first check the if we are inside the map bounds
    if ((pos.x >= 0) && (pos.x < MAP_WIDTH) && (pos.y >= 0) && (pos.y < MAP_HEIGHT)) {
        // check for level elements (like walls)
        // FIXME: 
        if (currentLevel->mapCells[pos.x][pos.y] == true) move = false;

        // check for any other entity, like monsters
        // for (ListElement *e = LIST_START (positions); e != NULL; e = e->next) {
        //     Position *p = (Position *) LIST_DATA (e);
        //     if (p->x == pos.x && p->y == pos.y) {
        //         // FIXME: how do we get the component associated with this motherfoca??
        //         // physics *phys = (Physics *) 
        //         move = false;
        //         break;
        //     }
        // }
    }   

    else move = false;

    return move;

}

#define UNSET   999

static i32 **dmap = NULL;

// 13/08/2018 -- simple representation of a Dijkstra's map, maybe later we will want a more complex map 
// or implement a more advance system
void generateTargetMap (i32 targetX, i32 targetY) {

    if (dmap != NULL) {
        for (short unsigned int i = 0; i < MAP_WIDTH; i++)
            free (dmap[i]);

        free (dmap);
    } 

    dmap = (i32 **) calloc (MAP_WIDTH, sizeof (i32 *));
    for (short unsigned int i = 0; i < MAP_WIDTH; i++)
        dmap[i] = (i32 *) calloc (MAP_HEIGHT, sizeof (i32));

    for (short unsigned int x = 0; x < MAP_WIDTH; x++)
        for (short unsigned int y = 0; y < MAP_HEIGHT; y++)
            dmap[x][y] = UNSET;

    dmap[targetX][targetY] = 0;

    bool changesMade = true;
    while (changesMade) {
        changesMade = false;

        for (short unsigned int x = 0; x < MAP_WIDTH; x++) {
            for (short unsigned int y = 0; y < MAP_HEIGHT; y++) {
                i32 currCellValue = dmap[x][y];
                // check cells around and update them if necessary
                if (currCellValue != UNSET) {
                    if ((!isWall (x + 1, y)) && (dmap[x + 1][y] > currCellValue + 1)) {
                        dmap[x + 1][y] = currCellValue + 1;
                        changesMade = true;
                    }

                    if ((!isWall (x - 1, y)) && (dmap[x - 1][y] > currCellValue + 1)) {
                        dmap[x - 1][y] = currCellValue + 1;
                        changesMade = true;
                    }

                    if ((!isWall (x, y - 1)) && (dmap[x][y - 1] > currCellValue + 1)) {
                        dmap[x][y - 1] = currCellValue + 1;
                        changesMade = true;
                    }

                    if ((!isWall (x, y + 1)) && (dmap[x][y + 1] > currCellValue + 1)) {
                        dmap[x][y + 1] = currCellValue + 1;
                        changesMade = true;
                    }
                }
            }
        }
    }

    // targetMap = dmap;

}


// 13/08/2018 -- 22:27 -- I don't like neither of these!
Position *getPos (i32 id) {

    for (ListElement *e = LIST_START (positions); e != NULL; e = e->next) {
        Position *pos = (Position *) LIST_DATA (e);
        if (pos->objectId == id) return pos;
    }

    return NULL;

}

// simple movement update for our entities based on the Dijkstra's map
// TODO: maybe we can create a more advanced system based on a state machine??
// TODO: this is a good place for multi-threading... I am so excited ofr that!!!
void updateMovement () {

    for (ListElement *e = LIST_START (movement); e != NULL; e = e->next) {
        Movement *mv = (Movement *) LIST_DATA (e);

        // determine if we are going to move this tick
        mv->ticksUntilNextMov -= 1;
        if (mv->ticksUntilNextMov <= 0) {
            Position *pos = getPos (mv->objectId);
            Position newPos = { .objectId = pos->objectId, .x = pos->x, .y = pos->y, .layer = pos->layer };

            // a monster should only move towards the player if it has seen him
            // and should chase him if we has been in the monster fov in the last 5 turns   
            bool chase = false;
            // player is visible
            if (fovMap[pos->x][pos->y] > 0) {
                chase = true;
                mv->chasingPlayer = true;
                mv->turnsSincePlayerSeen = 0;
            }

            // player is not visible, but see if we can still chase him...
            else {
                chase = mv->chasingPlayer;
                mv->turnsSincePlayerSeen += 1;
                if (mv->turnsSincePlayerSeen > 5) mv->chasingPlayer = false;
            }

            i32 speedCounter = mv->speed;
            while (speedCounter > 0) {
                // determine if we are in combat range with the player
                // TODO: add combat here!
                if ((fovMap[pos->x][pos->y] > 0) && (dmap[pos->x][pos->y] == 1)) {}
                // we are out of combat range, so get a new pos;
                else {
                    if (chase) {
                        Position moves[4];
                        unsigned int moveCounter = 0;
                        i32 currTargetValue = dmap[pos->x][pos->y];
                        if (dmap[pos->x - 1][pos->y] < currTargetValue) {
                            Position np = newPos;
                            np.x -= 1;
                            moves[moveCounter] = np;
                            moveCounter++;
                        }

                        if (dmap[pos->x][pos->y - 1] < currTargetValue) {
                            Position np = newPos;
                            np.y--;
                            moves[moveCounter] = np;
                            moveCounter++;
                        }

                        if (dmap[pos->x + 1][pos->y] < currTargetValue) {
                            Position np = newPos;
                            np.x += 1;
                            moves[moveCounter] = np;
                            moveCounter++;
                        }

                        if (dmap[pos->x][pos->y + 1] < currTargetValue) {
                            Position np = newPos;
                            np.y += 1;
                            moves[moveCounter] = np;
                            moveCounter++;
                        }

                        // pick a random potential pos
                        if (moveCounter > 0) {
                            u32 moveIdx = (u32) randomInt (0, moveCounter);
                            newPos = moves[moveIdx];
                        }

                    }

                    // if we are not chasing the player, we are in patrol state, so move randomly
                    else {
                        u32 dir = (u32) randomInt (0, 4);
                        switch (dir) {
                            case 0: newPos.x -= 1; break;
                            case 1: newPos.y -= 1; break;
                            case 2: newPos.x += 1; break;
                            case 3: newPos.y += 1; break;
                            default: break;
                        }
                    }

                    // check if we can move to the new pos
                    if (canMove (newPos)) {
                        // Updating the entity position in an odd way
                        pos->x = newPos.x;
                        pos->y = newPos.y;
                        mv->ticksUntilNextMov = mv->frecuency;
                    }

                    else mv->ticksUntilNextMov += 1;
                }

                speedCounter -= 1;
            }

        }

    }

}


// 14/08/2018 -- 23:02 -- test function to spawn some monsters to test their behaivour
void createMonster (GameObject *go) {

    // This is just a placeholder until it spawns in the world
    Position pos = { .x = 0, .y = 0, .layer = TOP_LAYER };
    addComponent (go, POSITION, &pos);

    Graphics g = { 0, 'M', 0xFFFFFFFF, 0x000000FF };
    addComponent (go, GRAPHICS, &g);

    // TODO: what physics do we want our monsters to have?
    // Physics phys = { 0, true, true };
    // addComponent (go, PHYSICS, &phys);

    Movement mv = { .speed = 1, .frecuency = 1, .ticksUntilNextMov = 1, .chasingPlayer = false, .turnsSincePlayerSeen = 0 };
    addComponent (go, MOVEMENT, &mv);

}


/*** COMBAT ***/




/*** MANAGER ***/

extern void calculateFov (u32 xPos, u32 yPos, u32 (*fovmap)[MAP_HEIGHT]);

// we will have the game update every time the player moves...
void updateGame (void) {

    if (playerTookTurn) {
        // fprintf (stdout, "Updating game!\n");
        Position *playerPos = (Position *) getComponent (player, POSITION);
        generateTargetMap (playerPos->x, playerPos->y);
        updateMovement ();

        playerTookTurn = false;
    }

    // recalculate the fov
    if (recalculateFov) {
        Position *playerPos = (Position *) getComponent (player, POSITION);
        calculateFov (playerPos->x, playerPos->y, fovMap);
        recalculateFov = false;
    }

}


/*** GAME MANAGER ***/

#include <assert.h>
#include <pthread.h>

#include "blackrock.h"
#include "game.h"
#include "player.h"
#include "item.h"
#include "map.h"

#include "objectPool.h"

#include "utils/myUtils.h"
#include "utils/dlist.h"

#include "ui/gameUI.h"  // for the message log

#include "config.h"     // for getting the data

#include "network/client.h"
#include "utils/log.h"

/*** WORLD STATE ***/

// TODO: create an object pool of list elements for forward optimization??

// Components
DoubleList *gameObjects = NULL;
DoubleList *positions = NULL;
DoubleList *graphics = NULL;
DoubleList *physics = NULL;
DoubleList *movement = NULL;
DoubleList *combat = NULL;
DoubleList *loot = NULL;

// Pools
Pool *goPool = NULL;
Pool *posPool = NULL;
Pool *graphicsPool = NULL;
Pool *physPool = NULL;
Pool *movePool = NULL;
Pool *combatPool = NULL;
Pool *lootPool = NULL;

// Player
extern Player *player;
bool playerTookTurn = false;

// Level 
Level *currentLevel = NULL;

// Score
Score *playerScore = NULL;

// FOV
u32 fovMap[MAP_WIDTH][MAP_HEIGHT];
bool recalculateFov = false;
extern void calculateFov (u32 xPos, u32 yPos, u32 [MAP_WIDTH][MAP_HEIGHT]);

extern void die (char *);

/*** INITIALIZATION **/

#pragma region INIT GAME

void *getGameData (void *data) {

    // retrieves the data from the items db
    initItems ();

    // connect to enemies db
    void connectEnemiesDb (void);
    connectEnemiesDb ();

}

// This should only be called once!
// Inits the global state of the game
// Inits all the data and structures for an initial game
void initGame (void) {

    pthread_t dataThread;

    // if (pthread_create (&dataThread, NULL, getGameData, NULL) != THREAD_OK) 
    //     die ("Error creating data thread!\n");

    // retrieves the data from the items db
    initItems ();

    // connect to enemies db
    void connectEnemiesDb (void);
    connectEnemiesDb ();

    gameObjects = dlist_init (free);
    positions = dlist_init (free);
    graphics = dlist_init (free);
    physics = dlist_init (free);
    movement = dlist_init (free);
    combat = dlist_init (free);
    loot = dlist_init (free);

    // init our pools
    goPool = initPool ();
    posPool = initPool ();
    graphicsPool = initPool ();
    physPool = initPool ();
    movePool = initPool ();
    combatPool = initPool ();
    lootPool = initPool ();

    // init the message log
    messageLog = dlist_init (free);

    // if (pthread_join (dataThread, NULL) != THREAD_OK) die ("Error joinning data thread!\n");

    fprintf (stdout, "Creating world...\n");

    void initWorld (void);
    initWorld ();

}

void *playerLogic (void *arg) {

    if (player != NULL) destroyPlayer ();
    
    player = createPlayer ();

    initPlayer (player);

}

// TODO: this inits the game to the tavern/village
// TODO: this can be a good place to check if we have a save file of a map and load that from disk
void initWorld (void) {

    pthread_t playerThread;

    if (pthread_create (&playerThread, NULL, playerLogic, NULL) != THREAD_OK) 
        die ("Error creating player thread!\n");

    // score struct, probably we will need a more complex system later
    playerScore = (Score *) malloc (sizeof (Score));
    void resetScore (void);
    resetScore ();

    // TODO: we will want to load the in game menu (tavern)

    // 20/08/2018 -- 22:51 -- as we don't have the tavern an the main menu ready, we will go staright into a new
    // game inside the dungeon
    void enterDungeon (void);
    enterDungeon ();

    if (pthread_join (playerThread, NULL) != THREAD_OK) die ("Error joinning player thread!\n");

    // we can now place the player and we are done 
    Point playerSpawnPos = getFreeSpot (currentLevel->mapCells);
    player->pos->x = (u8) playerSpawnPos.x;
    player->pos->y = (u8) playerSpawnPos.y;

    calculateFov (player->pos->x, player->pos->y, fovMap);
    
}

#pragma endregion

/*** Game Object Management **/

#pragma region GAME OBJECTS

// 11/08/2018 -- we assign a new id to each new GO
u32 newId = 0;

GameObject *createGO (void) {

    GameObject *go = NULL;

    // first check if there is an available one in the pool
    if (POOL_SIZE (goPool) > 0) {
        go = (GameObject *) pop (goPool);
        if (go == NULL) go = (GameObject *) malloc (sizeof (GameObject));
    } 
    else go = (GameObject *) malloc (sizeof (GameObject));

    if (go != NULL) {
        go->id = newId;
        newId++;
        for (u8 i = 0; i < COMP_COUNT; i++) go->components[i] = NULL;
        dlist_insert_after (gameObjects, LIST_END (gameObjects), go);
    }
    
    return go;

}

void addComponent (GameObject *go, GameComponent type, void *data) {

    if (go == NULL || data == NULL) return;

    switch (type) {
        case POSITION: {
            if (getComponent (go, type) != NULL) return;

            Position *newPos = NULL;

            if ((POOL_SIZE (posPool) > 0)) newPos = (Position *) pop (posPool);
            else newPos = (Position *) malloc (sizeof (Position));

            Position *posData = (Position *) data;
            newPos->objectId = go->id;
            newPos->x = posData->x;
            newPos->y = posData->y;
            newPos->layer = posData->layer;

            go->components[type] = newPos;
            dlist_insert_after (positions, NULL, newPos);
        } break;
        case GRAPHICS: {
            if (getComponent (go, type) != NULL) return;

            Graphics *newGraphics = NULL;

            if ((POOL_SIZE (graphicsPool) > 0)) newGraphics = (Graphics *) pop (graphicsPool);
            else newGraphics = (Graphics *) malloc (sizeof (Graphics));

            Graphics *graphicsData = (Graphics *) data;
            newGraphics->objectId = go->id;
            newGraphics->name = (char *) calloc (strlen (graphicsData->name) + 1, sizeof (char));
            strcpy (newGraphics->name, graphicsData->name);
            newGraphics->glyph = graphicsData->glyph;
            newGraphics->fgColor = graphicsData->fgColor;
            newGraphics->bgColor = graphicsData->bgColor;

            go->components[type] = newGraphics;
            dlist_insert_after (graphics, NULL, newGraphics);
        } break;
        case PHYSICS: {
            if (getComponent (go, type) != NULL) return;

            Physics *newPhys = NULL;

            if ((POOL_SIZE (physPool) > 0)) newPhys = (Physics *) pop (physPool);
            else newPhys = (Physics *) malloc (sizeof (Physics));

            Physics *physData = (Physics *) data;
            newPhys->objectId = go->id;
            newPhys->blocksSight = physData->blocksSight;
            newPhys->blocksMovement = physData->blocksMovement;

            go->components[type] = newPhys;
            dlist_insert_after (physics, NULL, newPhys);
        } break;
        case MOVEMENT: {
            if (getComponent (go, type) != NULL) return;

            Movement *newMove = NULL;

            if ((POOL_SIZE (movePool) > 0)) newMove = (Movement *) pop (movePool);
            else newMove = (Movement *) malloc (sizeof (Movement));

            Movement *movData = (Movement *) data;
            newMove->objectId = go->id;
            newMove->speed = movData->speed;
            newMove->frecuency = movData->frecuency;
            newMove->ticksUntilNextMov = movData->ticksUntilNextMov;
            newMove->chasingPlayer = movData->chasingPlayer;
            newMove->turnsSincePlayerSeen = movData->turnsSincePlayerSeen;

            go->components[type] = newMove;
            dlist_insert_after (movement, NULL, newMove);
        } break;
        case COMBAT: {
            if (getComponent (go, type) != NULL) return;

            Combat *newCombat = NULL;

            if ((POOL_SIZE (combatPool) > 0)) newCombat = (Combat *) pop (combatPool);
            else newCombat = (Combat *) malloc (sizeof (Combat));

            Combat *combatData = (Combat *) data;
            newCombat->objectId = go->id;
            newCombat->baseStats = combatData->baseStats;
            newCombat->attack = combatData->attack;
            newCombat->defense = combatData->defense;

            go->components[type] = newCombat;
            dlist_insert_after (combat, NULL, newCombat);
        } break;
        case EVENT: {
            if (getComponent (go, type) != NULL) return;
            Event *newEvent = (Event *) malloc (sizeof (Event));
            Event *eventData = (Event *) data;
            newEvent->objectId = go->id;
            newEvent->callback = eventData->callback;

            go->components[type] = newEvent;
        }
        case LOOT: {
            if (getComponent (go, type) != NULL) return;
            Loot *newLoot = NULL;

            if (POOL_SIZE (lootPool) > 0) newLoot = (Loot *) pop (lootPool);
            else newLoot = (Loot *) malloc (sizeof (Loot));

            Loot *lootData = (Loot *) data;
            newLoot->objectId = go->id;
            newLoot->money[0] = lootData->money[0];
            newLoot->money[1] = lootData->money[1];
            newLoot->money[2] = lootData->money[2];
            newLoot->lootItems = lootData->lootItems;

            go->components[type] = newLoot;
            dlist_insert_after (loot, NULL, newLoot);
        } break;

        // We have an invalid GameComponent type, so don't do anything
        default: break;
    }

}

void removeComponent (GameObject *go, GameComponent type) {

    if (go == NULL) return;

    switch (type) {
        case POSITION: {
            Position *posComp = (Position *) getComponent (go, type);
            if (posComp == NULL) return;
            ListElement *e = dlist_get_ListElement (positions, posComp);
            void *posData = NULL;
            if (e != NULL) posData = dlist_remove_element (positions, e);
            push (posPool, posData);
            go->components[type] = NULL;
        } break;
        case GRAPHICS: {
            Graphics *graComp = (Graphics *) getComponent (go, type);
            if (graComp == NULL) return;
            ListElement *e = dlist_get_ListElement (graphics, graComp);
            void *graData = NULL;
            if (e != NULL) graData = dlist_remove_element (graphics, e);
            push (graphicsPool, graData);
            go->components[type] = NULL;
        } break;
        case PHYSICS: {
            Physics *physComp = (Physics *) getComponent (go, type);
            if (physComp == NULL) return;
            ListElement *e = dlist_get_ListElement (physics, physComp);
            void *physData = NULL;
            if (e != NULL) physData = dlist_remove_element (physics, e);
            push (physPool, physData);
            go->components[type] = NULL;
        } break;
        case MOVEMENT: {
            Movement *moveComp = (Movement *) getComponent (go, type);
            if (moveComp == NULL) return;
            ListElement *e = dlist_get_ListElement (movement, moveComp);
            void *moveData = NULL;
            if (e != NULL) moveData = dlist_remove_element (movement, e);
            push (movePool, moveData);
            go->components[type] = NULL;
        } break;
        case COMBAT: {
            Combat *combatComp = (Combat *) getComponent (go, type);
            if (combatComp == NULL) return;
            ListElement *e = dlist_get_ListElement (combat, combatComp);
            void *combatData = NULL;
            if (e != NULL) combatData = dlist_remove_element (combat, e);
            push (combatPool, combatData);
            go->components[type] = NULL;
        } break;
        case LOOT: {
            Loot *lootComp = (Loot *) getComponent (go, type);
            if (lootComp == NULL) return;
            ListElement *e = dlist_get_ListElement (loot, lootComp);
            void *lootData = NULL;
            if (e != NULL) lootData = dlist_remove_element (loot, e);
            push (lootPool, lootData);
            go->components[type] = NULL;
        } break;
        case EVENT: {
            Event *event = (Event *) getComponent (go, type);
            if (event != NULL) {
                free (event);
                go->components[type] = NULL;
            }
        }

        default: break;
    }

}

void *getComponent (GameObject *go, GameComponent type) { return go->components[type]; }

// TESTING 19/08/2018 -- 19:04
// FIXME: HOW CAN WE MANAGE THE WALLS!!!!???
DoubleList *getObjectsAtPos (u32 x, u32 y) {

    Position *pos = NULL;
    DoubleList *retVal = dlist_init (free);
    for (ListElement *e = LIST_START (gameObjects); e != NULL; e = e->next) {
        pos = (Position *) getComponent ((GameObject *) e->data, POSITION);
        if (pos != NULL)
            if (pos->x == x && pos->y == y) dlist_insert_after (retVal, NULL, e->data);
    }

    return retVal;

}

GameObject *searchGameObjectById (u32 id) {

    GameObject *go = NULL;
    for (ListElement *e = LIST_START (gameObjects); e != NULL; e = e->next) {
        go = (GameObject *) e->data;
        if (go != NULL) {
            if (go->id == id) return go;
        }
        
    }

    return NULL;

}

// This calls the Object pooling to deactive the go and have it in memory 
// to reuse it when we need it
void destroyGO (GameObject *go) {

    // remove all of the gos components
    for (u8 i = 0; i < COMP_COUNT; i++) removeComponent (go, i);

    // now send the go to the pool
    ListElement *e = dlist_get_ListElement (gameObjects, go);
    if (e != NULL) {
        void *data = dlist_remove_element (gameObjects, e);
        if (data != NULL) push (goPool, data);
    }

}

#pragma endregion

#pragma region CLEANUP

void cleanUpGame (void) {

    // clean up the player
    destroyPlayer ();
    fprintf (stdout, "Done cleanning up player.\n");

    // clean up our lists
    dlist_destroy (gameObjects);
    dlist_destroy (positions);
    dlist_destroy (graphics);
    dlist_destroy (physics);
    dlist_destroy (movement);
    dlist_destroy (combat);

    fprintf (stdout, "Done cleanning up lists.\n");

    void destroyLoot (void);
    destroyLoot ();

    fprintf (stdout, "Done cleanning up loot.\n");
    
    // cleanup the pools
    clearPool (goPool);
    clearPool (posPool);
    clearPool (graphicsPool);
    clearPool (physPool);
    clearPool (movePool);
    clearPool (combatPool);

    fprintf (stdout, "Done cleaning up pools.\n");

    // clean up items
    cleanUpItems ();
    fprintf (stdout, "Done cleaning up items.\n");

    // clean up enemies memory and db
    void cleanUpEnemies (void);
    cleanUpEnemies ();
    fprintf (stdout, "Done cleaning up enemies.\n");

    // clean up pathfinding structs
    void cleanTargetMap (void);
    cleanTargetMap ();

    free (currentLevel->mapCells);
    free (currentLevel);
    fprintf (stdout, "Done cleaning up map.\n");

    // clear all leaderboard data
    void cleanLeaderBoardData (void);
    cleanLeaderBoardData ();
    fprintf (stdout, "Done cleaning up leaderboard data.\n");

    fprintf (stdout, "Done cleaning up game!\n");

}

#pragma endregion

/*** MOVEMENT ***/

#pragma region MOVEMENT

bool isWall (u32 x, u32 y) { return (currentLevel->mapCells[x][y]); }

bool canMove (Position pos, bool isPlayer) {

    bool move = true;

    // first check the if we are inside the map bounds
    if ((pos.x >= 0) && (pos.x < MAP_WIDTH) && (pos.y >= 0) && (pos.y < MAP_HEIGHT)) {
        // check for level elements (like walls)
        if (isWall (pos.x, pos.y)) move = false;

        // check for any other entity, like monsters
        GameObject *go = NULL;
        Position *p = NULL;
        for (ListElement *e = LIST_START (gameObjects); e != NULL; e = e->next) {
            go = (GameObject *) e->data;
            p = (Position *) getComponent (go, POSITION);
            if (p->x == pos.x && p->y == pos.y) {
                if (((Physics *) getComponent (go, PHYSICS))->blocksMovement) move = false;
                break;
            }
        }

        // check for player
        if (!isPlayer) {
            if (pos.x == player->pos->x && pos.y == player->pos->y)
                move = false;

        }
    }   

    else move = false;

    return move;

}

#define UNSET   999

static i32 **dmap = NULL;

i32 **initTargetMap (void) {

    i32 **tmap = NULL;

    tmap = (i32 **) calloc (MAP_WIDTH, sizeof (i32 *));
    for (u8 i = 0; i < MAP_WIDTH; i++)
        tmap[i] = (i32 *) calloc (MAP_HEIGHT, sizeof (i32));

    return tmap;

}

void cleanTargetMap (void) {

    if (dmap != NULL) {
        for (u8 i = 0; i < MAP_WIDTH; i++)
            free (dmap[i]);

        free (dmap);
    } 

}

// 13/08/2018 -- simple representation of a Dijkstra's map, maybe later we will want a more complex map 
// or implement a more advance system
void generateTargetMap (i32 targetX, i32 targetY) {

    if (dmap == NULL) dmap = initTargetMap ();

    // reset the target map
    for (u8 x = 0; x < MAP_WIDTH; x++)
        for (u8 y = 0; y < MAP_HEIGHT; y++)
            dmap[x][y] = UNSET;

    dmap[targetX][targetY] = 0;

    bool changesMade = true;
    while (changesMade) {
        changesMade = false;

        for (u8 x = 0; x < MAP_WIDTH; x++) {
            for (u8 y = 0; y < MAP_HEIGHT; y++) {
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
// TODO: maybe a more efficient way is to only update the movement of the
// enemies that are in a close range?

void fight (Combat *att, Combat *def, bool isPlayer);

void updateMovement (void) {

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
                if ((fovMap[pos->x][pos->y] > 0) && (dmap[pos->x][pos->y] == 1)) {
                    // fight the player!
                    GameObject *enemy = searchGameObjectById (mv->objectId);
                    if (enemy != NULL) 
                        fight ((Combat *) getComponent (enemy, COMBAT), player->combat, false);
                }
                
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
                    if (canMove (newPos, false)) {
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

#pragma endregion

/*** ENEMIES ***/

#pragma region ENEMIES

#include <sqlite3.h>

// The path is form the makefile
const char *enemiesDbPath = "./data/enemies.db";
sqlite3 *enemiesDb;

// this is only used for development purposes
void createEnemiesDb (void) {

    char *err_msg = 0;

    // create monsters table
    // char *sql = "DROP TABLE IF EXISTS Monsters;"
    //             "CREATE TABLE Monsters(Id INT, Name TEXT, Probability DOUBLE)";

    // // create combat stats table
    // sql = "DROP TABLE IF EXISTS Combat;"
    //       "CREATE TABLE Combat(Id INT, Health INT, Armour INT, Dps INT, Strength INT, Hitchance INT, Speed INT, Critical INT, Dodge INT, Parry INT, Block INT)";

    // // create movement stats table
    // sql = "DROP TABLE IF EXISTS Movement;"
    //       "CREATE TABLE Movement(Id INT, Speed INT, Frequency INT)";

    // // create graphics table
    // sql = "DROP TABLE IF EXISTS Graphics;"
    //       "CREATE TABLE Graphics(Id INT, Glyph INT, Color TEXT)";

    // if (sqlite3_exec (enemiesDb, sql, 0, 0, &err_msg) != SQLITE_OK) {
    //     fprintf (stderr, "Error! Failed to create GRAPHICS table!\n");
    //     fprintf (stderr, "SQL error: %s\n", err_msg);
    //     sqlite3_free (err_msg);
    // }

    // else fprintf (stdout, "Table created!\n");

    // 06/09/2018 -- first approach to a drop system
    // drops
    // char *sql = "DROP TABLE IF EXISTS Drops;"
    //             "CREATE TABLE Drops(Id INT, Min INT, Max INT, Items TEXT)";

    // if (sqlite3_exec (enemiesDb, sql, 0, 0, &err_msg) != SQLITE_OK) {
    //     fprintf (stderr, "Error! Failed to create Drops table!\n");
    //     fprintf (stderr, "SQL error: %s\n", err_msg);
    //     sqlite3_free (err_msg);
    // }

}

typedef struct {

    u8 minGold;
    u8 maxGold;
    u32 *drops;
    u8 dropCount;

} MonsterLoot;

// 04/09/2018 -- 12:18
typedef struct {

    u32 id;
    char *name;
    double probability;
    MonsterLoot loot;

} Monster;

#define MONSTER_COUNT       9

DoubleList *enemyData = NULL;

u8 loadMonsterLoot (u32 monId, MonsterLoot *loot) {

    // get the db data
    sqlite3_stmt *res;
    char *sql = "SELECT * FROM Drops WHERE Id = ?";

    if (sqlite3_prepare_v2 (enemiesDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, monId);
    else {
        fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (enemiesDb));
        return 1;
    } 

    int step = sqlite3_step (res);

    loot->minGold = (u32) sqlite3_column_int (res, 1);
    loot->maxGold = (u32) sqlite3_column_int (res, 2);

    // get the possible drop items
    const char *c = sqlite3_column_text (res, 3);
    if (c != NULL) {
        char *itemsTxt = (char *) calloc (strlen (c) + 1, sizeof (char));
        strcpy (itemsTxt, c);

        // parse the string by commas
        char **tokens = splitString (itemsTxt, ',');
        if (tokens) {
            // count how many elements will be extracted
            u32 count = 0;
            while (*itemsTxt++) if (',' == *itemsTxt) count++;

            count += 1;     // take into account the last item after the last comma

            loot->dropCount = count;
            loot->drops = (u32 *) calloc (count, sizeof (u32));
            u32 idx = 0;
            for (u32 i = 0; *(tokens + i); i++) {
                loot->drops[idx] = atoi (*(tokens + i));
                idx++;
                free (*(tokens + i));
            }

            free (tokens);
        }

        // no drop items in the db or an error somewhere retrieving the data
        // so only hanlde the loot with the money
        else {
            loot->drops = NULL;
            loot->dropCount = 0;
        } 
    }
    
    else {
        loot->drops = NULL;
        loot->dropCount = 0;
    } 

    sqlite3_finalize (res);

    return 0;

}

// Getting the enemies data from the db into memory
static int loadEnemyData (void *data, int argc, char **argv, char **azColName) {
   
    Monster *mon = (Monster *) malloc (sizeof (Monster));

    mon->id = atoi (argv[0]);
    char temp[20];
    strcpy (temp, argv[1]);
    mon->name = (char *) calloc (strlen (temp) + 1, sizeof (char));
    strcpy (mon->name, temp);
    mon->probability = atof (argv[2]);

    if (loadMonsterLoot (mon->id, &mon->loot) != 0)
        fprintf (stderr, "Error getting monster loot. Monster id: %i.\n", mon->id);

    dlist_insert_after (enemyData, LIST_END (enemyData), mon);

   return 0;

}

void connectEnemiesDb () {

    // connect to the items db
    if (sqlite3_open (enemiesDbPath, &enemiesDb) != SQLITE_OK) {
        fprintf (stderr, "%s\n", sqlite3_errmsg (enemiesDb));
        die ("Problems with enemies db!\n");
    } 
    else fprintf (stdout, "Succesfully connected to the enemies db.\n");

    // createEnemiesDb ();

    // enemies in memory
    enemyData = dlist_init (free);

    // load the enemies data into memory
    char *err = 0;
    char *sql = "SELECT * FROM Monsters";
          
    if (sqlite3_exec (enemiesDb, sql, loadEnemyData, NULL, &err) != SQLITE_OK ) {
        fprintf (stderr, "SQL error: %s\n", err);
        sqlite3_free (err);
    } 
    
    else fprintf(stdout, "Done loading monster data.\n");
                                
}

Monster *searchMonById (u32 monId) {

    Monster *mon = NULL;
    for (ListElement *e = LIST_START (enemyData); e != NULL; e = e->next) {
        mon = (Monster *) e->data;
        if (mon->id == monId) break;
    }

    if (mon == NULL) fprintf (stderr, "No monster found with id: %i\n", monId);

    return mon;

}

u8 addGraphicsToMon (u32 monId, Monster *monData, GameObject *mon) {

    // get the db data
    sqlite3_stmt *res;
    char *sql = "SELECT * FROM Graphics WHERE Id = ?";

    if (sqlite3_prepare_v2 (enemiesDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, monId);
    else {
        fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (enemiesDb));
        return 1;
    } 

    int step = sqlite3_step (res);

    asciiChar glyph = (asciiChar) sqlite3_column_int (res, 1);
    char *name = (char *) calloc (strlen (monData->name) + 1, sizeof (char));
    strcpy (name, monData->name);
    const char *c = sqlite3_column_text (res, 2);
    char *colour = (char *) calloc (strlen (c) + 1, sizeof (char));
    strcpy (colour, c);
    u32 color = (u32) xtoi (colour);
    Graphics g = { 0, glyph, color, 0x000000FF, false, false, name };
    addComponent (mon, GRAPHICS, &g);

    free (name);
    free (colour);
    sqlite3_finalize (res);

    return 0;

}

u8 addMovementToMon (u32 monId, Monster *monData, GameObject *mon) {

    // get the db data
    sqlite3_stmt *res;
    char *sql = "SELECT * FROM Movement WHERE Id = ?";

    if (sqlite3_prepare_v2 (enemiesDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, monId);
    else {
        fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (enemiesDb));
        return 1;
    } 

    int step = sqlite3_step (res);

    u32 speed = (u32) sqlite3_column_int (res, 1);
    u32 frecuency = (u32) sqlite3_column_int (res, 2);
    Movement mv = { .speed = speed, .frecuency = frecuency, .ticksUntilNextMov = 1, .chasingPlayer = false, .turnsSincePlayerSeen = 0 };
    addComponent (mon, MOVEMENT, &mv);

    sqlite3_finalize (res);

    return 0;

}

u8 addCombatToMon (u32 monId, Monster *monData, GameObject *mon) {

    // get the db data
    sqlite3_stmt *res;
    char *sql = "SELECT * FROM Combat WHERE Id = ?";

    if (sqlite3_prepare_v2 (enemiesDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, monId);
    else {
        fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (enemiesDb));
        return 1;
    } 

    int step = sqlite3_step (res);

    Combat c;

    c.baseStats.strength = (u32) sqlite3_column_int (res, 4);
    c.attack.baseDps = (u32) sqlite3_column_int (res, 3);
    c.attack.hitchance = (u32) sqlite3_column_int (res, 5);
    c.attack.attackSpeed = (u32) sqlite3_column_int (res, 6);
    c.attack.spellPower = 0;
    c.attack.criticalStrike = (u32) sqlite3_column_int (res, 7);
    c.defense.armor = (u32) sqlite3_column_int (res, 2);
    c.defense.dodge = (u32) sqlite3_column_int (res, 8);
    c.defense.parry = (u32) sqlite3_column_int (res, 9);
    c.defense.block = (u32) sqlite3_column_int (res, 10);

    c.baseStats.maxHealth = ((u32) sqlite3_column_int (res, 1)) + c.defense.armor;
    c.baseStats.health = c.baseStats.maxHealth;

    addComponent (mon, COMBAT, &c);

    sqlite3_finalize (res);

    return 0; 

}

// NEW way for creating the monsters using the enemies db and the in game memory enemy list
GameObject *createMonster (u32 monId) {

    // get the memory data
    Monster *monData = searchMonById (monId);
    if (monData == NULL) {
        fprintf (stderr, "Error! No monster found with the provided id!\n");
        return NULL;
    }

    GameObject *mon = createGO ();
    mon->dbId = monId;

    // This is just a placeholder until it spawns in the world
    Position pos = { .x = 0, .y = 0, .layer = MID_LAYER };
    addComponent (mon, POSITION, &pos);

    // graphics
    if (addGraphicsToMon (monId, monData, mon) != 0) {
        fprintf (stderr, "Error adding graphics component!\n");
        return NULL;
    } 

    Physics phys = { 0, true, false };
    addComponent (mon, PHYSICS, &phys);

    // movement
    if (addMovementToMon (monId, monData, mon) != 0) {
        fprintf (stderr, "Error adding movement component!\n");
        return NULL;
    } 

    // combat
    if (addCombatToMon (monId, monData, mon) != 0) {
        fprintf (stderr, "Error adding combat component!\n");
        return NULL;
    } 

    return mon;

}

// 22/08/2018 -- 7:11 -- this is our first random monster generation function
// TODO: maybe later also take into account the current level
// FIXME: create a better system
u32 getMonsterId (void) {

    // number of monster per type
    u8 weak = 2;
    u8 medium = 2;
    u8 strong = 3;
    u8 veryStrong = 2;

    u32 choice;

    switch (randomInt (1, 4)) {
        case 1:
            switch (randomInt (1, weak)) {
               case 1: choice = 101; break; 
               case 2: choice = 102; break; 
            }
            break;
        case 2:
            switch (randomInt (1, medium)) {
                case 1: choice = 201; break;
                case 2: choice = 202; break;
            }
            break;
        case 3:
            switch (randomInt (1, strong)) {
                case 1: choice = 301; break;
                case 2: choice = 302; break;
                case 3: choice = 303; break;
            }
            break;
        case 4:
            switch (randomInt (1, veryStrong)) {
                case 1: choice = 401; break;
                case 2: choice = 402; break;
            }
            break;
        default: break;
    }

    return choice;

}

void cleanUpEnemies (void) {

    dlist_destroy (enemyData);
    sqlite3_close (enemiesDb);

}

#pragma endregion

/*** LOOT - ITEMS ***/

#pragma region LOOT

// FIXME:
DoubleList *generateLootItems (u32 *dropItems, u32 count) {

    // FIXME: take into account that we have differente tables for weapons
    // and armour and for normal items
    // FIXME: 07/09/2018 -- 09:16 -- we can only create normal items
    // connect to the items db to get items probability

    // get each items probability to calculate drops
    // double itemsProbs[count];

    // sqlite3_stmt *res;
    // char *sql = "SELECT * FROM Items WHERE Id = ?";
    // for (u32 i = 0; i < count; i++) {
    //     if (sqlite3_prepare_v2 (enemiesDb, sql, -1, &res, 0) == SQLITE_OK)
    //         sqlite3_bind_int (res, 1, dropItems[i]);

    //     // FIXME: better error handling
    //     // else {
    //     //     fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (enemiesDb));
    //     //     return 1;
    //     // } 
    // }

    // sqlite3_finalize (res);

    // generate a random number of loot items
    // FIXME: this will be based depending of the enemy
    u8 itemsNum = (u8) randomInt (0, 2);

    if (itemsNum == 0) return NULL;
    else {
        fprintf (stdout, "Creating loot items...\n");

        DoubleList *lootItems = dlist_init (free);

        // generate random loot drops based on items probability
        // FIXME: 07/09/2018 -- 09:44 this is just for testing
        // we are only slectig the first items in dropItems and ignoring probs
        // FIXME: fix the drop items index
        Item *item = NULL;
        u8 type;
        for (u8 i = 0; i < itemsNum; i++) {
            type = dropItems[i] / 1000;
            switch (type) {
                case 1: item = createItem (dropItems[i]); break;
                case 2: item = createWeapon (dropItems[i]); break;
                case 3: item = createArmour (dropItems[i]); break;
                default: break;
            }

            if (item) dlist_insert_after (lootItems, LIST_END (lootItems), item);
        }

        return lootItems;
    }

}

// FIXME:
// generate random loot based on the enemy
void *createLoot (void *arg) {

    GameObject *go = (GameObject *) arg;

    Monster *mon = searchMonById (go->dbId);

    if (mon != NULL) {
        fprintf (stdout, "Creating new loot...\n");

        Loot newLoot;

        // FIXME: create a better system
        // generate random money directly
        newLoot.money[0] = randomInt (mon->loot.minGold, mon->loot.maxGold);
        newLoot.money[1] = randomInt (0, 99);
        newLoot.money[2] = randomInt (0, 99);

        newLoot.lootItems = generateLootItems (mon->loot.drops, mon->loot.dropCount);

        // add the loot struct to the go as a component
        addComponent (go, LOOT, &newLoot);

        fprintf (stdout, "New loot created!\n");
    }

}

Loot *currentLoot = NULL;

bool emptyLoot (Loot *loot) {

    bool noItems, noMoney;

    if (loot->lootItems != NULL) {
      if (LIST_SIZE (loot->lootItems) <= 0) noItems = true;  
    } 
    else noItems = true;

    if (loot->money[0] == 0 && loot->money[1] == 0 && loot->money[2] == 0) noMoney = true;

    if (noItems && noMoney) return true;
    else return false;

}

void displayLoot (void *goData) {

    GameObject *go = NULL;
    if (goData != NULL) go = searchGameObjectById (((GameObject *)(goData))->id);

    if (go != NULL) {
        Loot *loot = (Loot *) getComponent (go, LOOT);
        if (loot != NULL) {
            if (!emptyLoot (loot)) {
                currentLoot = loot;
                toggleLootWindow ();
            }

            else {
                currentLoot = NULL;
                logMessage ("There is no loot here.", WARNING_COLOR);
            } 
            
        }   

        else {
            currentLoot = NULL;
            logMessage ("There is no loot here.", WARNING_COLOR);
        } 
    }
    
}

void collectGold (void) {

    if (currentLoot != NULL) {
        if (currentLoot->money[0] > 0 || currentLoot->money[1] > 0 || currentLoot->money[2] > 0) {
            char *str = createString ("You collected: %ig %is %ic",
                currentLoot->money[0], currentLoot->money[1], currentLoot->money[2]);
            
            if ((player->money[2] + currentLoot->money[2]) >= 100) {
                player->money[1] += 1;
                player->money[2] = 0;
                currentLoot->money[2] = 0;
            }

            else {
                player->money[2] += currentLoot->money[2];
                currentLoot->money[2] = 0;
            } 

            if ((player->money[1] + currentLoot->money[1]) >= 100) {
                player->money[0] += 1;
                player->money[1] = 0;
                currentLoot->money[1] = 0;
            }

            else {
                player->money[1] += currentLoot->money[1];
                currentLoot->money[1] = 0;
            }

            player->money[0] += currentLoot->money[0];
            currentLoot->money[0] = 0;

            logMessage (str, DEFAULT_COLOR);
            free (str);

        }

        else logMessage ("There is not gold here to collect!", WARNING_COLOR);

    }

}

void destroyLoot (void) {

    // clean up active loot objects
    if (loot != NULL) {
        if (LIST_SIZE (loot) > 0) {
            Loot *lr = NULL;
            for (ListElement *e = LIST_START (loot); e != NULL; e = e->next) {
                lr = (Loot *) e->data;
                if (lr->lootItems != NULL) {
                    if (LIST_SIZE (lr->lootItems) > 0) {
                        for (ListElement *le = LIST_START (lr->lootItems); le != NULL; le = le->next) 
                            dlist_remove_element (lr->lootItems, le);
                        
                    }

                    free (lr->lootItems);
                }   
            }
        }

        dlist_destroy (loot);
    }

    // clean up loot pool
    if (lootPool != NULL) {
        if (POOL_SIZE (lootPool) > 0) {
            Loot *lr = NULL;
            for (PoolMember *p = POOL_TOP (lootPool); p != NULL; p = p->next) {
                lr = (Loot *) p->data;
                if (lr->lootItems != NULL) {
                    if (LIST_SIZE (lr->lootItems) > 0) {
                        for (ListElement *le = LIST_START (lr->lootItems); le != NULL; le = le->next) 
                            dlist_remove_element (lr->lootItems, le);
                    }

                    free (lr->lootItems);
                }
            } 
        }
        clearPool (lootPool);

    }

}

#pragma endregion

/*** COMBAT ***/

#pragma region COMBAT

// FIXME: fix probability
char *calculateDefense (Combat *def, bool isPlayer) {

    GameObject *defender = NULL;
    if (isPlayer) defender = searchGameObjectById (def->objectId);

    u8 chance = (u8) randomInt (0, 3);
    char *msg = NULL;
    switch (chance) {
        case 0: break;
        case 1: {
            if (def->defense.dodge > 0) {
                u8 roll = (u8) randomInt (1, 100);
                if (roll <= def->defense.dodge) {
                    if (!isPlayer) logMessage ("You dodge the attack.", STOPPED_COLOR);
                    else {
                        Graphics *g = (Graphics *) getComponent (defender, GRAPHICS);
                        if (g != NULL) 
                            msg = createString ("The %s dodges your attack.", g->name);
                    }
                } 
            }
        } break;
        case 2: {
            if (def->defense.parry > 0) {
                u8 roll = (u8) randomInt (1, 100);
                if (roll <= def->defense.parry) {
                    if (!isPlayer) logMessage ("You parry the attack.", STOPPED_COLOR);
                    else {
                        Graphics *g = (Graphics *) getComponent (defender, GRAPHICS);
                        if (g != NULL) 
                            msg = createString ("The %s parries your attack.", g->name);
                    }  
                }
            }
        } break;
        case 3: {
            if (def->defense.block > 0) {
                u8 roll = (u8) randomInt (1, 100);
                if (roll <= def->defense.block) {
                    // FIXME: only block if you have a shield equipped
                    if (!isPlayer) logMessage ("You block the attack.", STOPPED_COLOR);
                    else {
                        Graphics *g = (Graphics *) getComponent (defender, GRAPHICS);
                        if (g != NULL) 
                            msg = createString ("The %s blocks your attack.", g->name);
                    }
                }  
            }
        } break;
        default: break;
    }

    return msg;

}

// FIXME:
u32 getPlayerDmg (Combat *att) {

    u32 damage;

    Item *mainWeapon = player->weapons[MAIN_HAND];
    if (mainWeapon != NULL) {
        Weapon *main = (Weapon *) getItemComponent (mainWeapon, WEAPON);
        if (main == NULL) {
            fprintf (stderr, "No weapon component!\n");
            return 0;
        } 

        // check if we are wielding 2 weapons
        Item *offWeapon = NULL;
        if (main->twoHanded == false) {
            if (player->weapons[OFF_HAND] != NULL) offWeapon = player->weapons[OFF_HAND];

            // FIXME: take into acount if it is a shield or whatever
            // FIXME: how do we calculate the damage?

            // FIXME: this is just for testing
            // if we only have one weapon
            if (offWeapon == NULL) damage = main->dps;
        }  

        // FIXME: this is only for testing
        // FIXME: also create a more dynamic damage
        // fighting with a two handed weapon
        else damage = main->dps;
    }

    // FIXME: add a more dynamic damage
    // we are fighting we our bare hands!
    else {
        if (att->attack.baseDps == 1) damage = randomInt (1, 2);
        else 
            damage = (u32) randomInt (att->attack.baseDps - (att->attack.baseDps / 2), att->attack.baseDps);
    }

    // FIXME: how do we want to handle strength
    damage += att->baseStats.strength;

    return damage;

}

u32 calculateDamage (Combat *att, Combat *def, bool isPlayer) {

    GameObject *attacker = NULL;
    GameObject *defender = NULL;
    if (isPlayer) defender = searchGameObjectById (def->objectId);
    else attacker = searchGameObjectById (att->objectId);

    u32 damage;

    if (isPlayer) damage = getPlayerDmg (att);
    // if the attacker is a mob, just get the the base dps + strength
    else {
        // damage = att->attack.baseDps;
        damage = (u32) randomInt (att->attack.baseDps - (att->attack.baseDps / 2), att->attack.baseDps);
        damage += att->baseStats.strength;
    }

    // take a roll to decide if we can hit a critical
    bool critical = false;
    if (randomInt (1, 100) <= att->attack.criticalStrike) {
        critical = true;
        damage *= 2;
    } 

    char *str = NULL;

    if (isPlayer) {
        Graphics *g = (Graphics *) getComponent (defender, GRAPHICS);
        str = createString ("You hit the %s for %i damage.", g->name, damage);
        if (critical) logMessage (str, CRITICAL_COLOR);
        else logMessage (str, HIT_COLOR);
    }

    else {
        Graphics *g = (Graphics *) getComponent (attacker, GRAPHICS);
        str = createString ("The %s hits you for %i damage!", g->name, damage);
        logMessage (str, DAMAGE_COLOR);
    }

    free (str);

    return damage;

}

void updatePlayerScore (GameObject *);

void checkForKill (GameObject *defender, bool isPlayer) {

    // check for monster kill
    if (isPlayer) {
        if (((Combat *) getComponent (defender, COMBAT))->baseStats.health <= 0) {
            // we want the player to be able to walk over the corpse
            ((Position *) getComponent (defender, POSITION))->layer = MID_LAYER; 
            Physics *phys = (Physics *) getComponent (defender, PHYSICS);
            phys->blocksMovement = false;
            phys->blocksSight = false;

            removeComponent (defender, MOVEMENT);

            pthread_t lootThread;

            if (pthread_create (&lootThread, NULL, createLoot, defender) != THREAD_OK)
                fprintf (stderr, "Error creating loot thread!\n");

            Event e = { 0, displayLoot };
            addComponent (defender, EVENT, &e);

            // TODO: maybe add a better visial feedback
            Graphics *gra = (Graphics *) getComponent (defender, GRAPHICS);
            gra->glyph = '%';
            gra->fgColor = 0x990000FF;
            char *str = createString ("You killed the %s.", gra->name);
            logMessage (str, KILL_COLOR);
            free (str);

            updatePlayerScore (defender);

            if (pthread_join (lootThread, NULL) != THREAD_OK)
                fprintf (stderr, "Error joinning loot thread!\n");

        }
    }

    // check for player death
    else {
        if (player->combat->baseStats.health <= 0) {
            logMessage ("You have died!!", KILL_COLOR);
            // TODO: player death animation?
            void gameOver (void);
            gameOver ();
        }
    }

}

// FIXME:
// TODO: 16/08/2018 -- 18:59 -- we can only handle melee weapons
void fight (Combat *att, Combat *def, bool isPlayer) {

    GameObject *attacker = NULL;
    GameObject *defender = NULL;
    if (isPlayer) defender = searchGameObjectById (def->objectId); 
    else attacker = searchGameObjectById (att->objectId);

    // FIXME: check for attack speed

    // check for the attack hit chance
    u32 hitRoll = (u32) randomInt (1, 100);
    if (hitRoll <= att->attack.hitchance) {
        // chance of blocking, parry or dodge
        char *msg = calculateDefense (def, isPlayer);
        if (msg == NULL) {
            // health = maxhealth = basehealth + armor
            def->baseStats.health -= calculateDamage (att, def, isPlayer);

            checkForKill (defender, isPlayer);
        }

        else {
            if (msg != NULL) {
                logMessage (msg, STOPPED_COLOR);
                free (msg);
            } 
        }

    }

    // The attcker missed the target
    else {
        if (isPlayer) logMessage ("Your attack misses.", MISS_COLOR);

        else {
            Graphics *g = (Graphics *) getComponent (attacker, GRAPHICS);
            char *str = createString ("The %s misses you.", g->name);
            logMessage (str, MISS_COLOR);
            free (str);
        }
    }

}

#pragma endregion

/*** LEVEL MANAGER ***/

#pragma region LEVEL MANAGER

void updateLeaderBoards (void);

// TODO: wrap all the things up before we return to the main menu
void returnToMenu (void) {

    // update the leaderboards files
    updateLeaderBoards ();

}

// FIXME:
// This is called every time we generate a new level to start fresh, only from data from the pool
void clearOldLevel (void) {

    newId = 0; // reset the go id

    void *data = NULL;

    if (gameObjects != NULL) {
        if (LIST_SIZE (gameObjects) > 0) {
            // send all of our objects and components to ther pools
            for (ListElement *e = LIST_START (gameObjects); e != NULL; e = e->next) {
                data = dlist_remove_element (gameObjects, e);
                if (data != NULL) destroyGO ((GameObject *) data);
            }
        }

    }

    if (items != NULL) {
        if (LIST_SIZE (items) > 0) {
            // send the items to their pool
            for (ListElement *e = LIST_START (items); e != NULL; e = e->next) {
                data = dlist_remove_element (items, e);
                if (data != NULL) destroyItem ((Item *) data);
            }
        }
    }

}

// game over logic
void gameOver (void) {  

    setActiveScene (postGameScreen ());

    destroyGameUI ();
    resetGameUI ();

}


// we will have the game update every time the player moves...
void *updateGame (void *data) {

    if (playerTookTurn) {
        generateTargetMap (player->pos->x, player->pos->y);
        updateMovement ();

        playerTookTurn = false;
    }

    // recalculate the fov
    if (recalculateFov) {
        calculateFov (player->pos->x, player->pos->y, fovMap);
        recalculateFov = false;
    }

}

// gets you into a nw level
void useStairs (void *goData) {

    currentLevel->levelNum += 1;

    void generateLevel (void);
    generateLevel ();

    calculateFov (player->pos->x, player->pos->y, fovMap);

    // TODO: what is our win condition?

    char *msg = createString ("You are now on level %i", currentLevel->levelNum);
    logMessage (msg, 0xFFFFFFFF);
    free (msg);

}

void placeStairs (Point spawn) {

    GameObject *stairs = createGO ();
    Position p = { 0, spawn.x, spawn.y, MID_LAYER };
    addComponent (stairs, POSITION, &p);
    Graphics g = { 0, '<', 0xFFD700FF, 0x00000000, false, true, "Stairs" };
    addComponent (stairs, GRAPHICS, &g);
    Physics phys = { 0, false, false };
    addComponent (stairs, PHYSICS, &phys);
    Event e = { 0, useStairs };
    addComponent (stairs, EVENT, &e);

}

// FIXME: 
void generateLevel (void) {

    // make sure we have cleaned the previous level data
    clearOldLevel ();

    // this is used to render the walls to the screen... but maybe it is not a perfect system
    initMap (currentLevel->mapCells);

    // TODO: create other map elements such as stairs
    // As of 20/08/2018 -- 23:18 -- we can only move through the dungeon using the stair cases
    // but we can only move forward, we can not return to the previous level
    // Point stairsPoint = getFreeSpot (currentLevel->mapCells);
    placeStairs (getFreeSpot (currentLevel->mapCells));

    fprintf (stdout, "Creating monsters...\n");

    // FIXME: how do we handle how many monsters to add
    u8 monNum = 15;
    u8 count = 0;
    for (u8 i = 0; i < monNum; i++) {
        // generate a random monster
        GameObject *monster = createMonster (getMonsterId ());
        if (monster != NULL) {
            // spawn in a random position
            Point monsterSpawnPos = getFreeSpot (currentLevel->mapCells);
            Position *monsterPos = (Position *) getComponent (monster, POSITION);
            monsterPos->x = (u8) monsterSpawnPos.x;
            monsterPos->y = (u8) monsterSpawnPos.y;
            // TODO: mark the spawnPos as filled
            count++;
        }
    }

    fprintf (stdout, "%i / %i monsters created successfully\n", count, monNum);

}

void enterDungeon (void) {

    if (currentLevel == NULL) {
        currentLevel = (Level *) malloc (sizeof (Level));
        currentLevel->levelNum = 1;
        currentLevel->mapCells = (bool **) calloc (MAP_WIDTH, sizeof (bool *));
        for (u8 i = 0; i < MAP_WIDTH; i++)
            currentLevel->mapCells[i] = (bool *) calloc (MAP_HEIGHT, sizeof (bool));

    } 
    
    // generate a random world froms scratch
    // TODO: maybe later we want to specify some parameters based on difficulty?
    // or based on the type of terrain that we want to generate.. we don't want to have the same algorithms
    // to generate rooms and for generating caves or open fields

    // after we have allocated the new level structure, we can start generating the first level
    generateLevel ();
    
    // TODO: add different texts here!!
    logMessage ("You have entered the dungeon!", 0xFFFFFFFF);

    fprintf (stdout, "You have entered the dungeon!\n");

}

// TODO: add a loading window??
// we reload the game scene as the same character
void retry (void) {

    if (!messageLog) messageLog = dlist_init (free);

    pthread_t playerThread;

    if (pthread_create (&playerThread, NULL, playerLogic, NULL) != THREAD_OK) 
        die ("Error creating player thread!\n");

    void resetScore (void);
    resetScore ();

    currentLevel->levelNum = 0;

    // create a new level
    enterDungeon ();

    if (pthread_join (playerThread, NULL) != THREAD_OK) die ("Error joinning player thread!\n");

    // we can now place the player and we are done 
    Point playerSpawnPos = getFreeSpot (currentLevel->mapCells);
    player->pos->x = (u8) playerSpawnPos.x;
    player->pos->y = (u8) playerSpawnPos.y;

    calculateFov (player->pos->x, player->pos->y, fovMap);

    fprintf (stdout, "Setting active scene...\n");
    setActiveScene (gameScene ());
    fprintf (stdout, "Destroying post gam screen...\n");
    destroyPostGameScreen ();

}

#pragma endregion

/*** SCORE ***/

#pragma region SCORE

LBEntry *playerLBEntry = NULL;

// we need to modify this when we add multiplayer i guess...
LBEntry *getPlayerLBEntry (void) {

    LBEntry *entry = (LBEntry *) malloc (sizeof (LBEntry));

    entry->playerName = (char *) calloc (strlen (player->name) + 1, sizeof (char));
    strcpy (entry->playerName, player->name);
    entry->completeName = createString ("%s the %s", player->name, getPlayerClassName (player->cClass));

    entry->nameColor = getPlayerClassColor (player->cClass);

    entry->level = createString ("%i", currentLevel->levelNum);

    entry->kills = createString ("%i", playerScore->killCount);

    entry->score = playerScore->score;
    // 26/09/2018 -- we are reversing the string for a better display in the UI
    entry->reverseScore = reverseString (createString ("%i", playerScore->score));

    return entry;

}

void updatePlayerScore (GameObject *monster) {

    playerScore->score += monster->dbId;

    playerScore->killCount++;

}

void resetScore (void) {

    playerScore->killCount = 0;
    playerScore->score = 0;

}

// TODO: later we will want to handle the logic of connecting to the server an retrieving data of
// the global leaderboards
void showScore (void) {

    // clean up the level 
    clearOldLevel ();

    // get player score ready for display
    playerLBEntry = getPlayerLBEntry ();

    // render score image with the current score struct
    toggleScoreScreen ();

    // delete death screen UI and image
    deleteDeathScreen ();

}

#pragma endregion

/*** MULTIPLAYER ***/

// TODO: 17/11/2018 -- do we want to create a new thread for each request we make?

// TODO: 17/11/2018 -- where do we want to put this?
Client *playerClient = NULL;

#pragma region MULTIPLAYER

// FIXME: how do we get the game type?
// called from the main menu to request a new game lobby
void multiplayer_createLobby (void) {

    if (!client_connectToServer (playerClient)) {
        if (!client_createLobby (playerClient, ARCADE)) {
            // TODO: move the player to the lobby screen
            // TODO: where do we recieve the data of the game lobby?
        }

        // TODO: give feedback to the player
        else logMsg (stderr, ERROR, CLIENT, "Failed to create a new game lobby!");
    }
    
    // TODO: give feedback to the player
    else logMsg (stderr, ERROR, CLIENT, "Failed to connect to server!");

}

// FIXME: how do we get the game type?
// called from the main menu to request to join a game lobby
void multiplayer_joinLobby (void) {

    if (!client_connectToServer (playerClient)) {
        if (!client_joinLobby (playerClient, ARCADE)) {
            // TODO: move the player to the lobby screen
            // TODO: where do we recieve the data of the game lobby?
        }
    }

    // TODO: give feedback to the player
    else logMsg (stderr, ERROR, CLIENT, "Failed to connect to server!");

}

#pragma end region

/*** LEADERBOARDS ***/

// FIXME: 17/11/2018 -- we need to update our logic to better work with the server!!

#pragma region LEADERBOARDS

// this are from the makefile
const char localLBFilePath[64] = "./data/localLB.cfg";
const char globalLBFilePath[64] = "./data/globalLB.cfg";

// FIXME: USE THIS!!!
bool changedLocalLB = false;

bool updateGlobalLb = false;

Config *localLBConfig = NULL;
Config *globalLBConfig = NULL;

DoubleList *localLBData = NULL;
DoubleList *globalLBData = NULL;

// get the config data into a list
// we expect the data to be already sorted!!
DoubleList *getLBData (Config *config) {

    DoubleList *lbData = dlist_init (free);

    ConfigEntity *entity = NULL;
    char *class = NULL;
    u8 c;

    for (ListElement *e = LIST_START (localLBConfig->entities); e != NULL; e = e->next) {
        entity = (ConfigEntity *) e->data;
        LBEntry *lbEntry = (LBEntry *) malloc (sizeof (LBEntry));
 
        lbEntry->playerName = getEntityValue (entity, "name");
        c = atoi (getEntityValue (entity, "class"));
        class = getPlayerClassName (c);

        if ((lbEntry->playerName != NULL) && (class != NULL)) 
            lbEntry->completeName = createString ("%s the %s", lbEntry->playerName, class);
        else if (lbEntry->playerName != NULL) {
            lbEntry->completeName = (char *) calloc (strlen (lbEntry->playerName) + 1, sizeof (char));
            strcpy (lbEntry->completeName, lbEntry->playerName);
        }
        // if some how the data got corrupted...
        else {
            lbEntry->playerName = (char *) calloc (10, sizeof (char));
            strcpy (lbEntry->playerName, "Anonymous");
            lbEntry->completeName = (char *) calloc (10, sizeof (char));
            strcpy (lbEntry->completeName, "Anonymous");
        }

        lbEntry->nameColor = getPlayerClassColor (c);

        lbEntry->level = getEntityValue (entity, "level");

        lbEntry->kills = getEntityValue (entity, "kills");

        // this is used for the sort
        char *score = getEntityValue (entity, "score");
        lbEntry->score = atoi (score);
        // 26/09/2018 -- we are reversing the string for a better display in the UI
        lbEntry->reverseScore = reverseString (score);

        dlist_insert_after (lbData, LIST_END (lbData), lbEntry);
        
        free (score);
    }

    return lbData;

}

DoubleList *getLocalLBData (void) {

    DoubleList *lbData = NULL;

    // check if we have a .conf file
    localLBConfig = parseConfigFile ("./data/localLB.cfg");
    if (localLBConfig != NULL) {
        lbData = getLBData (localLBConfig);

        // insert the current player score at the end no matter what
        dlist_insert_after (lbData, NULL, playerLBEntry);

        // then sort the list
        // lbData->start = mergeSort (LIST_START (lbData));
    } 

    else {
        // FIXME: we don't have a local leaderboard, so create one
    }

    // return the list for display
    return lbData;

}

// FIXME:
// FIXME: check the date of the file
// FIXME: server connection
DoubleList *getGlobalLBData (void) {

    DoubleList *globalData = NULL;

    // check if we have already a .conf file
    globalLBConfig = parseConfigFile ("./data/globalLB.cfg");
    if (globalLBConfig != NULL) {
        globalData = getLBData (globalLBConfig);

        // insert the player score no matter what
        dlist_insert_after (globalData, NULL, playerLBEntry);

        // then sort the list
        // globalData->start = mergeSort (LIST_START (globalData));
    } 

    // we don't have a global lb file, so connect to the server
    /* else {
        if (!connectedToServer) {
            // if (initConnection ()) {
            //     fprintf (stderr, "Failed to retrieve global LB!\n");
            //     // FIXME: give feedback to the player
            // }
        }

        // we are connected, so request the file
        if (makeRequest (REQ_GLOBAL_LB) != 0) fprintf (stderr, "Failed request!\n");
        else {
             // FIXME: check that we have got a valid file

            globalLBConfig = parseConfigFile ("./data/globalLB.cfg");
            if (globalLBConfig != NULL) {
                globalData = getLBData (globalLBConfig);

                // insert the player score no matter what
                dlist_insert_after (globalData, NULL, playerLBEntry);

                // then sort the list
                // globalData->start = mergeSort (LIST_START (globalData));
            } 
        } 

        closeConnection ();
    } */

    return globalData;

}

// FIXME:
Config *createNewLBCfg (DoubleList *lbData) {

    // make sure that we are only wrtitting the 10 best scores...
    // don't forget that the data is sorted like < 

    Config *cfg = (Config *) malloc (sizeof (Config));
    cfg->entities = dlist_init (free);

    u8 count = 0;
    ListElement *e = LIST_END (lbData);
    LBEntry *entry = NULL;
    while (count < 10 && e != NULL) {
        entry = (LBEntry *) e->data;

        ConfigEntity *newEntity = (ConfigEntity *) malloc (sizeof (ConfigEntity));
        newEntity->name = createString ("%s", "ENTRY");
        setEntityValue (newEntity, "name", entry->playerName);
        setEntityValue (newEntity, "class", createString ("%i", entry->charClass));
        setEntityValue (newEntity, "level", entry->level);
        setEntityValue (newEntity, "kills", entry->kills);
        setEntityValue (newEntity, "score", createString ("%i", entry->score));

        dlist_insert_after (cfg->entities, LIST_START (cfg->entities), newEntity);

        e = e->prev;
        count++;
    }

    return cfg;

}

// FIXME: server connection
// TODO: maybe make an async call to the server to upload the file
void updateLBFile (const char *filename, Config *cfg, bool globalLB) {   

    // write out the cfg file
    writeConfigFile (filename, cfg);

    /* if (globalLB) {
        if (!connectedToServer) {
            if (initConnection ()) {
                fprintf (stderr, "Failed to retrieve global LB!\n");
                // FIXME: give feedback to the player
            }
        }

        // we are connected, so post the file
        if (makeRequest (POST_GLOBAL_LB) != 0) fprintf (stderr, "Failed post request!\n");

        closeConnection ();
    }*/

}

void updateLeaderBoards (void) {

    // update local
    updateLBFile (localLBFilePath, createNewLBCfg (localLBData), false);

    if (updateGlobalLb)
        updateLBFile (globalLBFilePath, createNewLBCfg (globalLBData), true);

}

void deleteLBEntry (LBEntry *entry) {

    if (entry != NULL) {
        if (entry->playerName) free (entry->playerName);
        if (entry->completeName) free (entry->completeName);
        if (entry->level) free (entry->level);
        if (entry->kills) free (entry->kills);
        if (entry->reverseScore) free (entry->reverseScore);

        free (entry);
    }

}

void destroyLeaderBoard (DoubleList *lb) {

    while (LIST_SIZE (lb) > 0) 
        deleteLBEntry ((LBEntry *) dlist_remove_element (lb, LIST_END (lb)));
        
    free (lb);

}

void cleanLeaderBoardData (void) {

    // delete config data
    if (localLBConfig) clearConfig (localLBConfig);
    if (globalLBConfig) clearConfig (globalLBConfig);

    // delete parsed data
    if (localLBData) destroyLeaderBoard (localLBData);
    if (globalLBData) destroyLeaderBoard (globalLBData);

}

#pragma endregion
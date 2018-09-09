/*** GAME MANAGER ***/

// 03/08/2018 -- 18:18
// For now this will be like our Game Controller, well kind of
// lets see how the game scales and the we will decide what to do

#include <assert.h>

#include "blackrock.h"
#include "game.h"
#include "player.h"
#include "item.h"
#include "map.h"

#include "objectPool.h"

#include "utils/myUtils.h"
#include "utils/list.h"

#include "ui/gameUI.h"  // for the message log

#include "config.h"     // for getting the data

/*** WORLD STATE ***/

// Components
List *gameObjects = NULL;
List *positions = NULL;
List *graphics = NULL;
List *physics = NULL;
List *movement = NULL;
List *combat = NULL;
List *loot = NULL;

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

// Configs
// Config *monsterConfig = NULL;

// FOV
u32 fovMap[MAP_WIDTH][MAP_HEIGHT];
bool recalculateFov = false;

extern void die (char *);

// This should only be called once!
// Inits the global state of the game
// Inits all the data and structures for an initial game
void initGame (void) {

    gameObjects = initList (free);
    positions = initList (free);
    graphics = initList (free);
    physics = initList (free);
    movement = initList (free);
    combat = initList (free);
    loot = initList (free);

    // init our pools
    goPool = initPool ();
    posPool = initPool ();
    graphicsPool = initPool ();
    physPool = initPool ();
    movePool = initPool ();
    combatPool = initPool ();
    lootPool = initPool ();

    // retrieves the data from the items db
    initItems ();

    // init the message log
    messageLog = initList (free);

    // getting the data
    // monsterConfig = parseConfigFile ("./data/monster.cfg");
    // if (monsterConfig == NULL) 
    //     die ("Critical Error! No monster config!\n");

    // connect to enemies db
    void connectEnemiesDb (void);
    connectEnemiesDb ();

    void initWorld (void);
    initWorld ();

}

// TODO: this inits the game to the tavern/village
// TODO: this can be a good place to check if we have a save file of a map and load that from disk
void initWorld (void) {

    player = initPlayer ();

    // TODO: we will want to load the in game menu (tavern)

    // 20/08/2018 -- 22:51 -- as we don't have the tavern an the main menu ready, we will go staright into a new
    // game inside the dungeon
    void enterDungeon (void);
    enterDungeon ();

    if (player->weapons[0] != NULL) {
        fprintf (stdout, "Got a weapon!\n");
    }
    else fprintf (stdout, "No weapon!\n");
    
}

/*** Game Object Management **/

// 08/08/2018 --> we now handle some GameObjects with a llist and a Pool;
// the map is managed using an array

// 08/08/2018 -- 22:14
// we start the program with no objects in the pool
static unsigned int inactive = 0;

// 11/08/2018 -- we will assign a new id to each new GO starting at 1
// id = 0 is the player 
unsigned int newId = 1;

GameObject *createGO (void) {

    GameObject *go = NULL;

    // first check if there is an available one in the pool
    if (POOL_SIZE (goPool) > 0) {
        go = pop (goPool);
        if (go == NULL) go = (GameObject *) malloc (sizeof (GameObject));
    } 
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

    if (go == NULL || data == NULL) return;

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
            newGraphics->name = (char *) calloc (strlen (graphicsData->name) + 1, sizeof (char));
            strcpy (newGraphics->name, graphicsData->name);
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
            insertAfter (physics, NULL, newPhys);
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
        case COMBAT: {
            if (getComponent (go, type) != NULL) return;

            Combat *newCombat = NULL;

            if ((POOL_SIZE (combatPool) > 0)) newCombat = pop (combatPool);
            else newCombat = (Combat *) malloc (sizeof (Combat));

            Combat *combatData = (Combat *) data;
            newCombat->objectId = go->id;
            newCombat->baseStats = combatData->baseStats;
            newCombat->attack = combatData->attack;
            newCombat->defense = combatData->defense;

            go->components[type] = newCombat;
            insertAfter (combat, NULL, newCombat);
        } break;
        case EVENT: {
            if (getComponent (go, type) != NULL) return;
            Event *newEvent = (Event *) malloc (sizeof (Event));
            Event *eventData = (Event *) data;
            newEvent->objectId = go->id;
            newEvent->callback = eventData->callback;

            go->components[type] = newEvent;

            // FIXME: do we need a list and a pool of events??
        }
        case LOOT: {
            if (getComponent (go, type) != NULL) return;
            Loot *newLoot = NULL;

            if (POOL_SIZE (lootPool) > 0) newLoot = pop (lootPool);
            else newLoot = (Loot *) malloc (sizeof (Loot));

            Loot *lootData = (Loot *) data;
            newLoot->objectId = go->id;
            newLoot->money[0] = lootData->money[0];
            newLoot->money[1] = lootData->money[1];
            newLoot->money[2] = lootData->money[2];
            newLoot->lootItems = lootData->lootItems;

            go->components[type] = newLoot;
            insertAfter (loot, NULL, newLoot);
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
            ListElement *e = getListElement (positions, posComp);
            void *posData = NULL;
            if (e != NULL) posData = removeElement (positions, e);
            push (posPool, posData);
            go->components[type] = NULL;
        } break;
        case GRAPHICS: {
            Graphics *graComp = (Graphics *) getComponent (go, type);
            if (graComp == NULL) return;
            ListElement *e = getListElement (graphics, graComp);
            void *graData = NULL;
            if (e != NULL) graData = removeElement (graphics, e);
            push (graphicsPool, graData);
            go->components[type] = NULL;
        } break;
        case PHYSICS: {
            Physics *physComp = (Physics *) getComponent (go, type);
            if (physComp == NULL) return;
            ListElement *e = getListElement (physics, physComp);
            void *physData = NULL;
            if (e != NULL) physData = removeElement (physics, e);
            push (physPool, physData);
            go->components[type] = NULL;
        } break;
        case MOVEMENT: {
            Movement *moveComp = (Movement *) getComponent (go, type);
            if (moveComp == NULL) return;
            ListElement *e = getListElement (movement, moveComp);
            void *moveData = NULL;
            if (e != NULL) moveData = removeElement (movement, e);
            push (movePool, moveData);
            go->components[type] = NULL;
        } break;
        case COMBAT: {
            Combat *combatComp = (Combat *) getComponent (go, type);
            if (combatComp == NULL) return;
            ListElement *e = getListElement (combat, combatComp);
            void *combatData = NULL;
            if (e != NULL) combatData = removeElement (combat, e);
            push (combatPool, combatData);
            go->components[type] = NULL;
        } break;
        case LOOT: {
            Loot *lootComp = (Loot *) getComponent (go, type);
            if (lootComp == NULL) return;
            ListElement *e = getListElement (loot, lootComp);
            void *lootData = NULL;
            if (e != NULL) lootData = removeElement (loot, e);
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

void *getComponent (GameObject *go, GameComponent type) {

    void *retVal = go->components[type];
    if (retVal == NULL) return NULL;
    else return retVal;

}

// TESTING 19/08/2018 -- 19:04
// FIXME: HOW CAN WE MANAGE THE WALLS!!!!???
List *getObjectsAtPos (u32 x, u32 y) {

    Position *pos = NULL;
    List *retVal = initList (free);
    for (ListElement *e = LIST_START (gameObjects); e != NULL; e = e->next) {
        pos = (Position *) getComponent ((GameObject *) e->data, POSITION);
        if (pos->x == x && pos->y == y) insertAfter (retVal, NULL, e->data);
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
    for (short unsigned int i = 0; i < COMP_COUNT; i++) 
        removeComponent (go, i);

    // now send the go to the pool
    ListElement *e = getListElement (gameObjects, go);
    if (e != NULL) {
        void *data = removeElement (gameObjects, e);
        push (goPool, data);
    }

}

void cleanUpGame (void) {

    // clean up the player
    destroyPlayer ();
    fprintf (stdout, "Done cleanning up player\n");

    // clean game ui
    cleanGameUI ();

    // clean up our lists
    destroyList (gameObjects);
    destroyList (positions);
    destroyList (graphics);
    destroyList (physics);
    destroyList (movement);
    destroyList (combat);

    // clean every list of items inside each loot object
    if (loot != NULL) {
        if (LIST_SIZE (loot) > 0) {
            for (ListElement *e = LIST_START (loot); e != NULL; e = e->next) {
                if (((Loot *)(e->data))->lootItems != NULL) {
                    if (LIST_SIZE (((Loot *)(e->data))->lootItems) > 0) {
                        for (ListElement *le = LIST_START (((Loot *)(e->data))->lootItems); le != NULL; le = le->next) 
                            removeElement (((Loot *)(e->data))->lootItems, le);
                        
                    }

                    free (((Loot *)(e->data))->lootItems);
                }   
            }
        }
        destroyList (loot);
    }

    fprintf (stdout, "Done cleanning up lists\n");
    
    // cleanup the pools
    clearPool (goPool);
    clearPool (posPool);
    clearPool (graphicsPool);
    clearPool (physPool);
    clearPool (movePool);
    clearPool (combatPool);

    // clean every list of items inside each loot object
    if (lootPool != NULL) {
        if (POOL_SIZE (lootPool) > 0) {
            for (PoolMember *p = POOL_TOP (lootPool); p != NULL; p = p->next) {
                if (((Loot *)(p->data))->lootItems != NULL) {
                    if (LIST_SIZE (((Loot *)(p->data))->lootItems) > 0) {
                        for (ListElement *le = LIST_START (((Loot *)(le->data))->lootItems); le != NULL; le = le->next) 
                            removeElement (((Loot *)(le->data))->lootItems, le);
                    }

                    free (((Loot *)(p->data))->lootItems);
                }
            } 
        }
        clearPool (lootPool);
    }

    fprintf (stdout, "Done cleanning up pools\n");

    // clean up items
    cleanUpItems ();
    fprintf (stdout, "Done cleanning up items\n");

    // clean up enemies memory and db
    void cleanUpEnemies (void);
    cleanUpEnemies ();
    fprintf (stdout, "Done cleanning up enemies\n");
    
    // cleanup the message log
    destroyList (messageLog);

    free (currentLevel->mapCells);
    free (currentLevel);

    // clear the configs
    // clearConfig (monsterConfig);

    fprintf (stdout, "Done cleanning up game!\n");

}

/*** MOVEMENT ***/

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
// TODO: maybe a more efficient way is to only update the movement of the
// enemies that are in a close range?

void fight (Combat *att, Combat *def, bool isPlayer);

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

/*** ENEMIES ***/

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

// 04/09/2018 -- 12:18
typedef struct {

    u32 id;
    char *name;
    double probability;

} Monster;

#define MONSTER_COUNT       9

List *enemyData = NULL;

// Getting the enemies data from the db into memory
static int loadEnemyData (void *data, int argc, char **argv, char **azColName) {
   
    Monster *mon = (Monster *) malloc (sizeof (Monster));

    mon->id = atoi (argv[0]);
    char temp[20];
    strcpy (temp, argv[1]);
    mon->name = (char *) calloc (strlen (temp) + 1, sizeof (char));
    strcpy (mon->name, temp);
    mon->probability = atof (argv[2]);

    insertAfter (enemyData, LIST_END (enemyData), mon);

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
    enemyData = initList (free);

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

    Physics phys = { 0, true, true };
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

    destroyList (enemyData);
    sqlite3_close (enemiesDb);

}

/*** LOOT - ITEMS ***/

// FIXME:
List *generateLootItems (u32 *dropItems, u32 count) {

    // FIXME: take into account that we have differente tables for weapons
    // and armour and for normal items
    // FIXME: 07/09/2018 -- 09:16 -- we can only create normal items
    // connect to the items db to get items probability

    // get each items probability to calculate drops
    double itemsProbs[count];

    sqlite3_stmt *res;
    char *sql = "SELECT * FROM Items WHERE Id = ?";
    for (u32 i = 0; i < count; i++) {
        if (sqlite3_prepare_v2 (enemiesDb, sql, -1, &res, 0) == SQLITE_OK)
            sqlite3_bind_int (res, 1, dropItems[i]);

        // FIXME: better error handling
        // else {
        //     fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (enemiesDb));
        //     return 1;
        // } 
    }

    // generate a random number of loot items
    // FIXME: this will be based depending of the enemy
    u8 itemsNum = (u8) randomInt (0, 2);

    if (itemsNum == 0) return NULL;
    else {
        fprintf (stdout, "Creating loot items!\n");

        List *lootItems = initList (free);

        // generate random loot drops based on items probability
        // FIXME: 07/09/2018 -- 09:44 this is just for testing
        // we are only slectig the first items in dropItems and ignoring probs
        // FIXME: fix the drop items index
        Item *item;
        for (u8 i = 0; i < itemsNum; i++) {
            item = createItem (dropItems[i]);
            if (item != NULL) insertAfter (lootItems, LIST_END (lootItems), item);

        }
            
        return lootItems;
    }

}

// FIXME:
// generate random loot based on the enemy
u8 createLoot (GameObject *go) {

    fprintf (stdout, "Creating new loot...\n");

    // get the enemy's db data
    sqlite3_stmt *res;
    char *sql = "SELECT * FROM Drops WHERE Id = ?";

    if (sqlite3_prepare_v2 (enemiesDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, go->dbId);
    else {
        fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (enemiesDb));
        return 1;
    } 

    int step = sqlite3_step (res);

    u32 minGold = (u32) sqlite3_column_int (res, 1);
    u32 maxGold = (u32) sqlite3_column_int (res, 2);

    Loot newLoot;

    // FIXME: create a better system
    // generate random money directly
    newLoot.money[0] = randomInt (minGold, maxGold);
    newLoot.money[1] = randomInt (0, 99);
    newLoot.money[2] = randomInt (0, 99);

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

            u32 *dropItems = (int *) calloc (count, sizeof (int));
            u32 idx = 0;
            for (u32 i = 0; *(tokens + i); i++) {
                dropItems[idx] = atoi (*(tokens + i));
                idx++;
                free (*(tokens + i));
            }

            free (tokens);

            newLoot.lootItems = generateLootItems (dropItems, count);
        }

        // no drop items in the db or an error somewhere retrieving the data
        // so only hanlde the loot with the money
        else newLoot.lootItems = NULL;
    }
    
    else newLoot.lootItems = NULL;

    // add the loot struct to the go as a component
    addComponent (go, LOOT, &newLoot);

    fprintf (stdout, "New loot created!\n");

    sqlite3_finalize (res);

    return 0;

}

Loot *currentLoot = NULL;

bool emptyLoot (Loot *loot) {

    bool noItems, noMoney;

    if (loot->lootItems != NULL) 
        if (LIST_SIZE (loot->lootItems) <= 0) noItems = true;


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


/*** COMBAT ***/

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

u32 calculateDamage (Combat *att, Combat *def, bool isPlayer) {

    GameObject *attacker = NULL;
    GameObject *defender = NULL;
    if (isPlayer) defender = searchGameObjectById (def->objectId);
    else attacker = searchGameObjectById (att->objectId);

    // get the damage
    u32 damage;
    // if the attacker is the player, search for weapon dps + strength
    if (isPlayer) {
        // FIXME: as of 01/09/2018 -- 03:47 -- we can only handle a two handed weapon
        // FIXME: also create a more dynamic damage
        // getting the weapon in the main hand
        // if (player->weapons[0] != NULL)
        //     damage = ((Weapon *) getItemComponent (player->weapons[0], WEAPON))->dps;

        // 21/08/2018 -- 23:25 -- this is for a more dynamic experience
        damage = (u32) randomInt (att->attack.baseDps - (att->attack.baseDps / 2), att->attack.baseDps);

        damage += att->baseStats.strength;
    }
    // if the attacker is a mob, just get the the base dps + strength
    else {
        // damage = att->attack.baseDps;
        damage = (u32) randomInt (att->attack.baseDps - (att->attack.baseDps / 2), att->attack.baseDps);
        damage += att->baseStats.strength;
    }

    // take a roll to decide if we can hit a critical
    u32 critical = (u32) randomInt (1, 100);
    bool crit = false;
    if (critical <= att->attack.criticalStrike) {
        crit = true;
        damage *= 2;
    } 

    if (isPlayer) {
        Graphics *g = (Graphics *) getComponent (defender, GRAPHICS);
        char *str = createString ("You hit the %s for %i damage.", g->name, damage);
        if (crit) logMessage (str, CRITICAL_COLOR);
        else logMessage (str, HIT_COLOR);
        free (str);
    }

    else {
        Graphics *g = (Graphics *) getComponent (attacker, GRAPHICS);
        char *str = createString ("The %s hits you for %i damage!", g->name, damage);
        if (crit) logMessage (str, DAMAGE_COLOR);
        else logMessage (str, DAMAGE_COLOR);
        free (str);
    }

    return damage;

}

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

            // FIXME: better error handling
            if (createLoot (defender) != 0) fprintf (stderr, "Error creating loot!\n");

            Event e = { 0, displayLoot };
            addComponent (defender, EVENT, &e);

            // TODO: maybe add a better visial feedback
            Graphics *gra = (Graphics *) getComponent (defender, GRAPHICS);
            gra->glyph = '%';
            gra->fgColor = 0x990000FF;
            char *str = createString ("You killed the %s.", gra->name);
            logMessage (str, KILL_COLOR);
            free (str);
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


/*** LEVEL MANAGER ***/

// This is called every time we generate a new level to start fresh, only from data from the pool
void clearOldLevel (void) {

    if (gameObjects == NULL) return;

    // send all of our objects and components to ther pools
    for (ListElement *e = LIST_START (gameObjects); e != NULL; e = e->next)
        destroyGO ((GameObject *) e->data);

}

// FIXME:
// As of 9/08/2018 -- 17:00 -- we only asks the player if he wants to play again
// TODO: maybe later we can first display a screen with a score and then ask him to play again
void gameOver (void) {



}

extern void calculateFov (u32 xPos, u32 yPos, u32 [MAP_WIDTH][MAP_HEIGHT]);

// we will have the game update every time the player moves...
void updateGame (void) {

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

    // FIXME: generate a new level taking into a account the new level num I guess
    void generateLevel (void);
    generateLevel ();

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
void generateLevel () {

    // make sure we have cleaned the previous level data
    clearOldLevel ();

    // this is used to render the walls to the screen... but maybe it is not a perfect system
    initMap (currentLevel->mapCells);

    // TODO: create other map elements such as stairs
    // As of 20/08/2018 -- 23:18 -- we can only move through the dungeon using the stair cases
    // but we can only move forward, we can not return to the previous level
    // Point stairsPoint = getFreeSpot (currentLevel->mapCells);
    placeStairs (getFreeSpot (currentLevel->mapCells));

    // FIXME: how do we handle how many monster sto add
    u8 monNum = 10;
    // FIXME: better error handling
    fprintf (stdout, "Creating monsters...\n");
    u16 count = 0;
    for (short unsigned int i = 0; i < monNum; i++) {
        // generate a random monster
        // FIXME: create a better system
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

        else fprintf (stderr, "Error creating monster!\n");
        
    }
    fprintf (stdout, "%i / %i monsters created successfully\n", count, monNum);

    // FIXME:
    // 20/08/2018 -- 17:24 -- our items are broken until we have a config file

    // finally, we have a map full with monsters and items,
    // so we can place the player and we are done 
    fprintf (stdout, "Spawning the player...\n");
    Point playerSpawnPos = getFreeSpot (currentLevel->mapCells);
    player->pos->x = (u8) playerSpawnPos.x;
    player->pos->y = (u8) playerSpawnPos.y;

}

// This should only run when we enter the dungeon directly from the menu / taver or village
// IT SHOULD NOT BE CALLED FROM INSIDE THE DUNGEON!!
void enterDungeon (void) {

    if (currentLevel == NULL) {
        currentLevel = (Level *) malloc (sizeof (Level));
        currentLevel->levelNum = 1;
        currentLevel->mapCells = (bool **) calloc (MAP_WIDTH, sizeof (bool *));
        for (short unsigned int i = 0; i < MAP_WIDTH; i++)
            currentLevel->mapCells[i] = (bool *) calloc (MAP_HEIGHT, sizeof (bool));

        // currentLevel->levelLoot = initList (free);
    } 
    
    // generate a random world froms scratch
    // TODO: maybe later we want to specify some parameters based on difficulty?
    // or based on the type of terrain that we want to generate.. we don't want to have the same algorithms
    // to generate rooms and for generating caves or open fields

    // after we have allocated the new level structure, we can start generating the first level
    generateLevel ();

    fprintf (stdout, "Done initializing game!\n");
    logMessage ("You have entered the dungeon!", 0xFFFFFFFF);

}

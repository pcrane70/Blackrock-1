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

#include "ui/gameUI.h"  // for the message log

#include "config.h"     // for getting the data

#define SUCCESS_COLOR   0x009900FF
#define WARNING_COLOR   0x990000FF


/*** WORLD STATE ***/

// Components
List *gameObjects = NULL;
List *positions = NULL;
List *graphics = NULL;
List *physics = NULL;
List *movement = NULL;
List *combat = NULL;
List *items = NULL;

// Pools
Pool *goPool = NULL;
Pool *posPool = NULL;
Pool *graphicsPool = NULL;
Pool *physPool = NULL;
Pool *movePool = NULL;
Pool *combatPool = NULL;
Pool *itemsPool = NULL;

// Player
GameObject *player = NULL;
// TODO: as of 0/08/2018 -- 17:14 -- we don't support items stacks in the inventory
Player *playerComp = NULL;  // for accessibility
bool playerTookTurn = false;

// Configs
Config *playerConfig = NULL;
Config *classesConfig = NULL;
Config *monsterConfig = NULL;
Config *itemsConfig = NULL;


// FOV
u32 fovMap[MAP_WIDTH][MAP_HEIGHT];
bool recalculateFov = false;

extern void die (void);

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
    items = initList (free);

    // init our pools
    goPool = initPool ();
    posPool = initPool ();
    graphicsPool = initPool ();
    physPool = initPool ();
    movePool = initPool ();
    combatPool = initPool ();
    itemsPool = initPool ();

    // init the message log
    messageLog = initList (free);

    // getting the data
    playerConfig = parseConfigFile ("./data/player.cfg");
    if (playerConfig == NULL) {
        fprintf (stderr, "Critical Error! No player config!\n");
        die ();
    }
    classesConfig = parseConfigFile ("./data/classes.cfg");
    if (classesConfig == NULL) {
        fprintf (stderr, "Critical Error! No classes config!\n");
        die ();
    }
    monsterConfig = parseConfigFile ("./data/monster.cfg");
    if (monsterConfig == NULL) {
        fprintf (stderr, "Critical Error! No monster config!\n");
        die ();
    }
    // TODO: do we need an appearance probability? 

    itemsConfig = parseConfigFile ("./data/items.cfg");
    if (itemsConfig == NULL) {
        fprintf (stderr, "Critical Error! No items config!\n");
        die ();
    }

    void initWorld (void);
    initWorld ();

}

// TODO: this inits the game to the tavern/village
// TODO: this can be a good place to check if we have a save file of a map and load that from disk
void initWorld (void) {

    GameObject *initPlayer (void);
    player = initPlayer ();
    playerComp = (Player *) getComponent (player, PLAYER);

    // TODO: we will want to load the in game menu (tavern)

    // 20/08/2018 -- 22:51 -- as we don't have the tavern an the main menu ready, we will go staright into a new
    // game inside the dungeon
    void enterDungeon (void);
    enterDungeon ();
    
}

/*** Game Object Management **/

// TODO: check for a save file to retrive the information freom there instead
// because the player is an special GO, we want to initialize him differently
GameObject *initPlayer (void) {

    GameObject *go = (GameObject *) malloc (sizeof (GameObject));
    go->id = 0;

    for (short unsigned int i = 0; i < COMP_COUNT; i++) go->components[i] = NULL;

    // This is just a placeholder until it spawns in the world
    Position pos = { .x = 0, .y = 0, .layer = TOP_LAYER };
    addComponent (go, POSITION, &pos);

    ConfigEntity *playerEntity = getEntityWithId (playerConfig, 1);

    Physics phys = { 0, true, true };
    addComponent (go, PHYSICS, &phys);

    Player p;
    p.name = getEntityValue (playerEntity, "name");
    p.genre = atoi (getEntityValue (playerEntity, "genre"));
    p.level = atoi (getEntityValue (playerEntity, "level"));

    // get class
    CharClass cClass = atoi (getEntityValue (playerEntity, "class"));
    p.cClass = cClass;
    ConfigEntity *classEntity = getEntityWithId (classesConfig, p.cClass);
    p.color = xtoi (getEntityValue (classEntity, "color"));

    p.maxWeight = atoi (getEntityValue (playerEntity, "maxWeight")) + atoi (getEntityValue (classEntity, "weightMod"));
    addComponent (go, PLAYER, &p);

    // As of 18/08/2018 -- 23-21 -- the color of the glyph is based on the class
    asciiChar glyph = atoi (getEntityValue (playerEntity, "glyph"));
    Graphics g = { 0, glyph, p.color, 0x000000FF };
    addComponent (go, GRAPHICS, &g);

    // TODO: modify the combat component based on the class
    // we need to have a file where we can read the stats we have saved
    // also we need to take into account that every class has different stats
    Combat c;
    c.baseStats.maxHealth = atoi (getEntityValue (playerEntity, "maxHealth"));
    c.baseStats.strength = atoi (getEntityValue (playerEntity, "strength"));
    c.attack.hitchance = 70;    // FIXME:
    c.attack.attackPower = 10;
    c.attack.spellPower = 0;
    c.attack.criticalStrike = 20;

    // FIXME: add defense
    c.defense.armor = 10;
    c.defense.block = 0;
    c.defense.dodge = 0;
    c.defense.parry = 0;

    addComponent (go, COMBAT, &c);

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

GameObject *createGO (void) {

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
            newGraphics->name = graphicsData->name;
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
            newCombat->baseStats = combatData->baseStats;
            newCombat->attack = combatData->attack;
            newCombat->defense = combatData->defense;

            go->components[type] = newCombat;
            insertAfter (combat, NULL, newCombat);
        } break;
        case ITEM: {
            if (getComponent (go, type) != NULL) return;

            Item *newItem = NULL;

            if ((POOL_SIZE (itemsPool) > 0)) newItem = pop (itemsPool);
            else newItem = (Item *) malloc (sizeof (Item));

            Item *itemData = (Item *) data;
            newItem->objectId = go->id;
            newItem->type = itemData->type;
            newItem->rarity = itemData->rarity;
            newItem->dps = itemData->dps;
            newItem->slot = itemData->slot;
            newItem->weight = itemData->weight;
            newItem->quantity = itemData->quantity;
            newItem->maxLifetime = itemData->maxLifetime;
            newItem->lifetime = newItem->maxLifetime;
            newItem->isEquipped = false;

            go->components[type] = newItem;
            insertAfter (items, NULL, newItem);
        } break;
        case PLAYER: {
            if (getComponent (go, type) != NULL) return;
            Player *newPlayer = (Player *) malloc (sizeof (Player));
            Player *playerData = (Player *) data;
            newPlayer->name = playerData->name;
            newPlayer->cClass = playerData->cClass;
            newPlayer->genre = playerData->genre;
            newPlayer->inventory = initList (free);
            newPlayer->level = playerData->level;
            newPlayer->maxWeight = playerData->maxWeight;

            go->components[type] = newPlayer;
            // TODO: maybe in multiplayer we will need a list of items
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

        // We have an invalid GameComponent type, so don't do anything
        default: break;
    }

}

void updateComponent (GameObject *go, GameComponent type, void *data) {

    if (go == NULL || data == NULL) return;

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
        case COMBAT: {
            Combat *cComp = (Combat *) getComponent (go, type);
            if (cComp == NULL) return;
            Combat *cData = (Combat *) data;
            cComp->baseStats = cData->baseStats;
            cComp->attack = cData->attack;
            cComp->defense = cData->defense;
        } break;
        case ITEM: {
            Item *itemComp = (Item *) getComponent (go, type);
            if (itemComp == NULL) return;
            Item *itemData = (Item *) data;
            itemComp->objectId = go->id;
            itemComp->type = itemData->type;
            itemComp->dps = itemData->dps;
            itemComp->slot = itemData->slot;
            itemComp->weight = itemData->weight;
            itemComp->quantity = itemData->quantity;
            itemComp->lifetime = itemData->lifetime;
            itemComp->isEquipped = itemData->isEquipped;
        } break;
        case PLAYER: {
            Player *playerComp = (Player *) getComponent (go, type);
            if (playerComp == NULL) return;
            Player *playerData = (Player *) data;
            playerComp->level = playerData->level;
            playerComp->maxWeight = playerData->maxWeight;
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
        case ITEM: {
            Item *itemComp = (Item *) getComponent (go, type);
            if (itemComp == NULL) return;
            ListElement *e = getListElement (items, itemComp);
            void *itemData = NULL;
            if (e != NULL) itemData = removeElement (items, e);
            push (itemsPool, itemData);
            go->components[type] = NULL;
        } break;

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

    if (LIST_SIZE (retVal) > 0) return retVal;
    else return NULL;

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

    // clean up our lists
    destroyList (gameObjects);
    destroyList (positions);
    destroyList (graphics);
    destroyList (physics);
    destroyList (movement);
    destroyList (combat);
    destroyList (items);

    // cleanup the pools
    clearPool (goPool);
    clearPool (posPool);
    clearPool (graphicsPool);
    clearPool (physPool);
    clearPool (movePool);
    clearPool (combatPool);
    clearPool (itemsPool);

    // cleanup the message log
    destroyList (messageLog);

    // clean up the player
    destroyList (((Player *) getComponent (player, PLAYER))->inventory);
    free (playerComp);

    // clear the configs
    clearConfig (playerConfig);
    clearConfig (classesConfig);
    clearConfig (monsterConfig);
    clearConfig (itemsConfig);

}

// This is called every time we generate a new level to start fresh, only from data from the pool
void clearOldLevel (void) {

    if (gameObjects == NULL) return;

    // send all of our objects and components to ther pools
    for (ListElement *e = LIST_START (gameObjects); e != NULL; e = e->next)
        destroyGO ((GameObject *) e->data);

}


/*** LEVEL MANAGER ***/

// 20/08/2018 -- 17:05 -- Testing this new function for creating items
GameObject *createItem (u8 itemId) {

    ConfigEntity *itemEntity = getEntityWithId (itemsConfig, itemId);
    if (itemEntity == NULL) return NULL;

    // we have a valid item, so create it...
    GameObject *item = createGO ();

    // this is just a placeholder
    Position pos = { .x = 0, .y = 0, .layer = LOWER_LAYER };
    addComponent (item, POSITION, &pos);

    asciiChar glyph = atoi (getEntityValue (itemEntity, "glyph"));
    u32 color = xtoi (getEntityValue (itemEntity, "color"));
    char *name = getEntityValue (itemEntity, "name");
    Graphics g = { 0, glyph, color, 0x000000FF, false, false, name };
    addComponent (item, GRAPHICS, &g);

    Physics phys = { .blocksMovement = false, .blocksSight = false };
    addComponent (item, PHYSICS, &phys);

    // FIXME:
    Item i;
    // i.type = atoi (getEntityValue (itemEntity, "type"));
    // i.rarity = atoi (getEntityValue (itemEntity, "rarity"));
    // i.quantity = atoi (getEntityValue (itemEntity, "quantity"));
    // i.weight = atoi (getEntityValue (itemEntity, "weight"));
    // i.maxLifetime = atoi (getEntityValue (itemEntity, "max_life"));
    // i.dps = atoi (getEntityValue (itemEntity, "dps"));

    i.type = 1;
    i.rarity = 1;
    i.quantity = 1;
    i.weight = 1;
    i.maxLifetime = 100;
    i.dps = 10;

    // FIXME:!!!!!!!
    i.slot = NULL;

    addComponent (item, ITEM, &i);
        
    return item;

}

/*** ITEMS ***/

// check how much the player is carrying in its inventory and equipment
u32 getCarriedWeight (void) {

    u32 weight = 0;
    Item *item = NULL;
    for (ListElement *e = LIST_START (playerComp->inventory); e != NULL; e = e->next) {
        item = (Item *) getComponent ((GameObject *) LIST_DATA (e), ITEM);
        if (item != NULL) weight += (item->weight * item->quantity);
    }

    return weight;

}

// An item is everithing a character can pickup and manipulate.
// It can be miscellaneous, a weapon, or a piece of equipment

// Function to get/pickup a nearby item
// As of 16/08/2018:
// The character must be on the same coord as the item to be able to pick it up
void getItem (void) {

    Position *playerPos = (Position *) getComponent (player, POSITION);
    // get a list of objects nearby the player
    List *objects = getObjectsAtPos (playerPos->x, playerPos->y);

    // we only pick one item each time
    GameObject *itemGO = NULL;
    Item *item = NULL;
    for (ListElement *e = LIST_START (objects); e != NULL; e = e->next) {
        itemGO = (GameObject *) LIST_DATA (e);
        // we have a valid item to pickup
        if ((item = (Item *) getComponent (itemGO, ITEM)) != NULL) break;
    }

    // check if we can actually pickup the item
    if (itemGO != NULL && item != NULL) {
        if ((getCarriedWeight () + item->weight) <= ((Player *) getComponent (player, PLAYER))->maxWeight) {
            // add the item to the inventory
            insertAfter (((Player *) getComponent (player, PLAYER))->inventory, NULL, itemGO);
            // remove the item from the map
            removeComponent (itemGO, POSITION);

            Graphics *g = (Graphics *) getComponent (itemGO, GRAPHICS);
            if (g != NULL) {
                char *msg = createString ("You picked up the %s.", g->name);
                logMessage (msg, SUCCESS_COLOR);
                free (msg);
            }

            playerTookTurn = true;
        }

        else logMessage ("You are carrying to much already!", WARNING_COLOR);
    }

    free (objects);

}

// TODO: how do we select which item to drop?
void dropItem (GameObject *go) {

    if (go == NULL) return;

    // check if we can drop the item at the current position
    Position *playerPos = (Position *) getComponent (player, POSITION);
    List *objects = getObjectsAtPos (playerPos->x, playerPos->y);
    bool canBeDropped = true;
    for (ListElement *e = LIST_START (objects); e != NULL; e = e->next) {
        // 16/08/2018 -- 20:16 -- for now if there is already an item in that spot,
        // you can NOT drop it
        Item *i = (Item *) getComponent ((GameObject *) e->data, ITEM);
        if (i != NULL) {
            canBeDropped = false;
            break;
        } 
    }

    Graphics *g = (Graphics *) getComponent (go, GRAPHICS);

    if (canBeDropped) {
        Position pos = { .x = playerPos->x, .y = playerPos->y, .layer = MID_LAYER };
        addComponent (go, POSITION, &pos);

        // FIXME: unequip item
        Item *item = (Item *) getComponent (go, ITEM);
        if (item->isEquipped) {}

        // remove from the inventory
        // FIXME:
        ListElement *e = getListElement (playerComp->inventory, go);
        if (e != NULL) removeElement (playerComp->inventory, e);

        if (g != NULL) {
            char *msg = createString ("You dropped the %s.", g->name);
            logMessage (msg, SUCCESS_COLOR);
            free (msg);
        }
        else logMessage ("You dropped the item.", SUCCESS_COLOR);
    }

    else {
        if (g != NULL) {
            char *msg = createString ("Can't drop the %s here.", g->name);
            logMessage (msg, WARNING_COLOR);
            free (msg);
        }
        else logMessage ("Can't drop the item here.", WARNING_COLOR);
    }

    free (objects);

}

// FIXME: how do we select which item to equip?
// equips an item, but only if it as a piece of equipment
// TODO: how do we unequip an item?
void equipItem (GameObject *go) {

    if (go == NULL) return;

    // TODO: check that the item can be equipped, if its is a weapon or a piece of armor

    Item *item = (Item *) getComponent (go, ITEM);
    if (item != NULL) {
        // TODO: how do we check which pice of armor it is??
        // TODO: unequip the item in the corresponding equipment slot

        // equip the item
        item->isEquipped = !item->isEquipped;

        // TODO: update the player stats based on the new item
        // Combat *itemCombat = (Combat *) getComponent (item, COMBAT);
        // Combat *playerCombat = (Combat *) getComponent (player, COMBAT);
        
    }

}

// TODO: consumables

// TODO: crafting

// TODO: what other actions maybe want for items?

// only reduce lifetime of weapons, and equipment
// TODO: I think we will want to add the ability to repair your items in a shop,
// but only if they are above 0, if you don't repair your items soon enough, 
// you will lose them
void updateLifeTime () {

    GameObject *go = NULL;
    Item *item = NULL;
    for (ListElement *e = LIST_START (playerComp->inventory); e != NULL; e = e->next) {
        go = (GameObject *) LIST_DATA (e);
        item = (Item *) getComponent (go, ITEM);
        // FIXME: only for weapons and armor
        item->lifetime--;

        if (item->lifetime <= 0) {
            bool wasEquipped = false;
            if (item->isEquipped) {
                wasEquipped = true;
                // FIXME: unequip item
            }

            // remove from inventory
            // FIXME:

            // TODO: give feedback to the player with messages in the log
        }
    }

}

/*** MOVEMENT ***/

bool isWall (u32 x, u32 y) { return (currentLevel->mapCells[x][y]); }

bool canMove (Position pos) {

    bool move = true;

    // first check the if we are inside the map bounds
    if ((pos.x >= 0) && (pos.x < MAP_WIDTH) && (pos.y >= 0) && (pos.y < MAP_HEIGHT)) {
        // check for level elements (like walls)
        if (currentLevel->mapCells[pos.x][pos.y] == true) move = false;

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
GameObject *createMonster (void) {

    GameObject *monster = NULL;

    // FIXME: do we want appearance prob for monsters??

    ConfigEntity *monEntity = NULL;
    // FIXME: this is just for testing -- 18/08/2018 -- 21:35
    if ((monEntity = getEntityWithId (monsterConfig, (u8) randomInt (1, 9))) != NULL) {
        monster = createGO ();

        // This is just a placeholder until it spawns in the world
        Position pos = { .x = 0, .y = 0, .layer = MID_LAYER };
        addComponent (monster, POSITION, &pos);

        asciiChar glyph = atoi (getEntityValue (monEntity, "glyph"));
        u32 color = xtoi (getEntityValue (monEntity, "color"));
        char *name = getEntityValue (monEntity, "name");
        Graphics g = { 0, glyph, color, 0x000000FF, false, false, name };
        addComponent (monster, GRAPHICS, &g);

        Physics phys = { 0, true, true };
        addComponent (monster, PHYSICS, &phys);

        // FIXME:
        // u32 speed = atoi (getEntityValue (monEntity, "mv_speed"));
        // u32 frecuency = atoi (getEntityValue (monEntity, "mv_frequency"));
        Movement mv = { .speed = 1, .frecuency = 1, .ticksUntilNextMov = 1, .chasingPlayer = false, .turnsSincePlayerSeen = 0 };
        addComponent (monster, MOVEMENT, &mv);

        // FIXME:
        Combat c;

        // c.baseStats.maxHealth = atoi (getEntityValue (monEntity, "maxHP"));
        // c.baseStats.strength = atoi (getEntityValue (monEntity, "strength"));
        // c.attack.hitchance = atoi (getEntityValue (monEntity, "hitchance"));
        // c.attack.attackPower = atoi (getEntityValue (monEntity, "attack_power"));
        // c.attack.spellPower = atoi (getEntityValue (monEntity, "spell_power"));
        // c.attack.attackSpeed = atoi (getEntityValue (monEntity, "attack_speed"));
        // c.attack.criticalStrike = atoi (getEntityValue (monEntity, "critical"));
        // c.defense.armor = atoi (getEntityValue (monEntity, "armor"));
        // c.defense.dodge = atoi (getEntityValue (monEntity, "dodge"));
        // c.defense.parry = atoi (getEntityValue (monEntity, "parry"));
        // c.defense.block = atoi (getEntityValue (monEntity, "block"));

        c.baseStats.maxHealth = 100;
        c.baseStats.health = c.baseStats.maxHealth;
        c.baseStats.strength = 10;
        c.attack.hitchance = 50;
        c.attack.attackPower = 10;
        c.attack.spellPower = 0;
        c.attack.attackSpeed = 1;
        c.attack.criticalStrike = 50;
        c.defense.armor = 10;
        c.defense.block = 0;
        c.defense.dodge = 0;
        c.defense.parry = 0;

        addComponent (monster, COMBAT, &c);
    }

    return monster;

}


/*** COMBAT ***/

// FIXME:
void fight (GameObject *attacker, GameObject *defender) {

    Combat *att = (Combat *) getComponent (attacker, COMBAT);
    Combat *def = (Combat *) getComponent (defender, COMBAT);

    // TODO: how do we check for attack speed?
    // check for the attack hit chance
    u32 hitRoll = (u32) randomInt (1, 100);
    if (hitRoll <= att->attack.hitchance) {
        // first get the weapon dps
        // TODO: 16/08/2018 -- 18:59 -- we can only handle melee weapons
        // FIXME: we need a better way to get what ever the charcter is wielding
        // GameObject *go = NULL;
        // Item *i = NULL;
        // Item *weapon = NULL;
        // for (ListElement *e = LIST_START (playerComp->inventory); e != NULL; e = e->next) {
        //     go = (GameObject *) e->data;
        //     i = (Item *) getComponent (go, ITEM);
        //     if (i->wielding == true) {
        //         weapon = i;
        //         break;
        //     } 
        // }

        // generate the attack based on the attacked modifiers
        // TODO: check for independent class modifiers, for example:
        // check for attackPower for knights and spellPower for mages
        // u32 damage = weapon->dps;   // base damage
        u32 damage = 10;

        // add modifiers
        // take into account the base attack power
        // if it has a melee weapon, take into account the strenght
        damage += (att->baseStats.strength + att->attack.attackPower);

        // take a roll to decide if we can hit a critical
        u32 critical = (u32) randomInt (1, 100);
        if (critical <= att->attack.criticalStrike) damage << 1;    // doubles the damage

        // then deal the calculated damage to the defender with all the information needed
        // and calculate the % of damage taken 
        // FIXME: calculate the defense modifiers
        def->baseStats.health -= damage;

        // TODO: create better strings for parry, etc

        if (attacker == player) {
            Graphics *g = (Graphics *) getComponent (defender, GRAPHICS);
            char *str = createString ("You hit the %s for %i damage.", g->name, damage);
            logMessage (str, 0xCCCCCCFF);
            free (str);
        }

        else {
            Graphics *g = (Graphics *) getComponent (attacker, GRAPHICS);
            char *str = createString ("The %s hits you for %i damage!", g->name, damage);
            logMessage (str, 0xCCCCCCFF);
            free (str);
        }

        // check for the defenders health 
        if (def->baseStats.health <= 0) {
            if (defender == player) {
                logMessage ("You have died!!", 0xCC0000FF);
                // TODO: player death animation?
                void gameOver (void);
                gameOver ();
            }

            else {
                // TODO: maybe add a better visial feedback
                Graphics *gra = (Graphics *) getComponent (defender, GRAPHICS);
                gra->glyph = '%';
                gra->fgColor = 0x990000FF;

                Position *pos = (Position *) getComponent (defender, POSITION);
                pos->layer = MID_LAYER; // we want the player to be able to walk over it

                Physics *phys = (Physics *) getComponent (defender, PHYSICS);
                phys->blocksMovement = false;
                phys->blocksSight = false;

                // TODO: don't remove the death body until we move to the next level

                removeComponent (defender, MOVEMENT);

                Graphics *g = (Graphics *) getComponent (defender, GRAPHICS);
                char *str = createString ("You killed the %s.", g->name);
                logMessage (str, 0xFF9900FF)   ;
                free (str);
            }
        }

    }

    // The attcker missed the target
    else {
        if (attacker == player) logMessage ("Your attack misses.", 0xCCCCCCFF);

        else {
            Graphics *g = (Graphics *) getComponent (attacker, GRAPHICS);
            char *str = createString ("The %s misses you.", g->name);
            logMessage (str, 0xCCCCCCFF);
            free (str);
        }
    }

}


/*** MANAGER ***/

// As of 9/08/2018 -- 17:00 -- we only asks the player if he wants to play again
// TODO: maybe later we can first display a screen with a score and then ask him to play again
void gameOver (void) {



}

extern void calculateFov (u32 xPos, u32 yPos, u32 [MAP_WIDTH][MAP_HEIGHT]);

// we will have the game update every time the player moves...
void updateGame (void) {

    if (playerTookTurn) {
        Position *playerPos = (Position *) getComponent (player, POSITION);
        generateTargetMap (playerPos->x, playerPos->y);
        updateMovement ();
        // TODO: update lifetime

        playerTookTurn = false;
    }

    // recalculate the fov
    if (recalculateFov) {
        Position *playerPos = (Position *) getComponent (player, POSITION);
        calculateFov (playerPos->x, playerPos->y, fovMap);
        recalculateFov = false;
    }

}

// gets you into a nw level
void useStairs (void) {

    currentLevel->levelNum += 1;

    // FIXME: generate a new level taking into a account the new level num I guess
    void generateLevel (void);
    generateLevel ();

    // Position *playerPos = (Position *)game_object_get_component(player, COMP_POSITION);
    // fov_calculate(playerPos->x, playerPos->y, fovMap);
    // generate_target_map(playerPos->x, playerPos->y);

    // TODO: what is our win condition?

    char *msg = createString ("You are now on level %i", currentLevel->levelNum);
    logMessage (msg, 0xFFFFFFFF);
    free (msg);

    // logMessage ("Event triggered!!!", SUCCESS_COLOR);

}

void generateLevel () {

    // make sure we have cleaned the previous level data
    clearOldLevel ();

    // FIXME: THIS IS ONLY FOR DEBUGGING
    fprintf (stdout, "GO Pool: %i  ", goPool->size);
    fprintf (stdout, "Pos Pool: %i  ", posPool->size);
    fprintf (stdout, "Graphics Pool: %i  ", graphicsPool->size);
    fprintf (stdout, "Phys Pool: %i  ", physPool->size);
    fprintf (stdout, "Move Pool: %i  ", movePool->size);
    fprintf (stdout, "Combat Pool: %i  ", combatPool->size);
    fprintf (stdout, "Item Pool: %i  ", itemsPool->size);

    // this is used to render the walls to the screen... but maybe it is not a perfect system
    initMap (currentLevel->mapCells);

    // TODO: create other map elements such as stairs
    // As of 20/08/2018 -- 23:18 -- we can only move through the dungeon using the stair cases
    // but we can only move forward, we can not return to the previous level
    GameObject *stairs = createGO ();
    Point stairsPoint = getFreeSpot (currentLevel->mapCells);
    Position p = { 0, stairsPoint.x, stairsPoint.y, MID_LAYER };
    addComponent (stairs, POSITION, &p);
    Graphics g = { 0, '<', 0xFFD700FF, 0x00000000, false, true, "Stairs" };
    addComponent (stairs, GRAPHICS, &g);
    Physics phys = { 0, false, false };
    addComponent (stairs, PHYSICS, &phys);
    Event e = { 0, useStairs };
    addComponent (stairs, EVENT, &e);

    fprintf (stdout, "Creating monsters...\n");
    // 14/08/2018 -- 23:02 -- spawn some monsters to test how they behave
    GameObject *createMonster (void);
    for (short unsigned int i = 0; i < 10; i++) {
        GameObject *monster = createMonster ();
        // spawn in a random position
        Point monsterSpawnPos = getFreeSpot (currentLevel->mapCells);
        Position *monsterPos = (Position *) getComponent (monster, POSITION);
        monsterPos->x = (u8) monsterSpawnPos.x;
        monsterPos->y = (u8) monsterSpawnPos.y;
        // TODO: mark the spawnPos as filled
        // fprintf (stdout, "Created a new monster!\n");
    }

    // FIXME:
    // 20/08/2018 -- 17:24 -- our items are broken until we have a config file
    GameObject *createItem (u8);

    // finally, we have a map full with monsters, so we can place the player and we are done 
    fprintf (stdout, "Spawning the player...\n");
    Point playerSpawnPos = getFreeSpot (currentLevel->mapCells);
    Position *playerPos = (Position *) getComponent (player, POSITION);
    playerPos->x = (u8) playerSpawnPos.x;
    playerPos->y = (u8) playerSpawnPos.y;

    // FIXME: THIS IS ONLY FOR DEBUGGING
    fprintf (stdout, "GO Pool: %i  ", goPool->size);
    fprintf (stdout, "Pos Pool: %i  ", posPool->size);
    fprintf (stdout, "Graphics Pool: %i  ", graphicsPool->size);
    fprintf (stdout, "Phys Pool: %i  ", physPool->size);
    fprintf (stdout, "Move Pool: %i  ", movePool->size);
    fprintf (stdout, "Combat Pool: %i  ", combatPool->size);
    fprintf (stdout, "Item Pool: %i  ", itemsPool->size);

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

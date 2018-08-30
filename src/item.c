#include <string.h>

#include "blackrock.h"
#include "game.h"

#include "utils/list.h"
#include "objectPool.h"

#include "item.h"

#include "config.h"

#include "ui/gameUI.h" // for strings

#include "utils/myUtils.h"

/*** ITEMS DB ***/

#include <sqlite3.h>

// The path is form the makefile
const char *dbPath = "./data/items.db";
sqlite3 *itemsDb;

List *items = NULL;
Pool *itemsPool = NULL;

// our items db
Config *itemsConfig = NULL;

// items components
List *weapons = NULL;
List *equipment = NULL;

// 28/08/2018 -- 09:03
// I think we don't need a weapons or equipment pool because we are not moving so much
// memory with them

// static u32 itemsId = 0;

extern unsigned int newId;

extern void die (void);

void initItems (void) {

    items = initList (free);
    itemsPool = initPool ();

    // items db
    itemsConfig = parseConfigFile ("./data/items.cfg");
    if (itemsConfig == NULL) {
        fprintf (stderr, "Critical Error! No items config!\n");
        die ();
    }

    // connect to the items db
    int rc = sqlite3_open (dbPath, &itemsDb);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Critical Error! Cannot open database: %s\n", sqlite3_errmsg (itemsDb));
        die ();
    }

    else fprintf (stdout, "Succesfully connected to the items db.\n");

    // FIXME: THis is only for testing the db
    // we are filling up the db with some data
    // char *sql = "DROP TABLE IF EXISTS Food;"
    //             "CREATE TABLE Food(Id INT, Name TEXT, Glyph INT, Rarity INT, Stackable INT, Quantity INT, Gold INT, Silver INT, Copper INT);"
    //             "INSERT INTO Food VALUES(1001, 'Apple', 46, 1, 1, 1, 0, 1, 50);"
    //             "INSERT INTO Food VALUES(1002, 'Bread', 46, 1, 1, 1, 0, 1, 80);";

    // char *err_msg = 0;
    // rc = sqlite3_exec (itemsDb, sql, 0, 0, &err_msg);

    // if (rc != SQLITE_OK) {
    //     fprintf (stderr, "Error! Failed to create table!\n");
    //     fprintf (stderr, "SQL error: %s\n", err_msg);
    //     sqlite3_free (err_msg);
    // }

    // else fprintf (stdout, "Table food created successfully!\n");

    // int last_id = sqlite3_last_insert_rowid (itemsDb);
    // printf("The last Id of the inserted row is %d\n", last_id);

}

#define ID_COL          0
#define NAME_COL        1
#define GLYPH_COL       2
#define RARITRY_COL     3
#define STACKABLE_COL   4
#define QUANTITY_COL    5
#define GOLD_COL        6
#define SILVER_COL      7
#define COPPER_COL      8

Item *newItem (void) {

    Item *i = NULL;

    // check if there is a an available one in the items pool
    if (POOL_SIZE (itemsPool) > 0) {
        i = pop (itemsPool);
        if (i == NULL) i = (Item *) malloc (sizeof (Item));
    } 
    else i = (Item *) malloc (sizeof (Item));

    if (i != NULL) {
        i->itemId = newId;
        newId++;
        for (u8 u = 0; u < GAME_OBJECT_COMPS; u++) i->components[u] = NULL;
        for (u8 u = 0; u < ITEM_COMPS; u++) i->itemComps[u] = NULL;
        insertAfter (items, LIST_END (items), i);
    }

    return i;

}

void *getGameComponent (Item *item, GameComponent type) { return item->components[type]; }

void *getItemComponent (Item *item, ItemComponent type) { return item->itemComps[type]; }

void addGameComponent (Item *item, GameComponent type, void *data) {

    if (item == NULL || data == NULL) return;

    switch (type) {
        case POSITION: {
            if (getGameComponent (item, type) != NULL) return;
            Position *newPos = NULL;
            if (POOL_SIZE (posPool) > 0) {
                newPos = pop (posPool);
                if (newPos == NULL) newPos = (Position *) malloc (sizeof (Position));
            }
            else newPos = (Position *) malloc (sizeof (Position));

            Position *posData = (Position *) data;
            newPos->objectId = item->itemId;
            newPos->x = posData->x;
            newPos->y = posData->y;
            newPos->layer = posData->layer;

            item->components[type] = newPos;
            insertAfter (positions, NULL, newPos);
        } break;
        case GRAPHICS: {
            if (getGameComponent (item, type) != NULL) return;
            Graphics *newGraphics = NULL;
            if (POOL_SIZE (graphicsPool) > 0) {
                newGraphics = pop (graphicsPool);
                if (newGraphics == NULL) newGraphics = (Graphics *) malloc (sizeof (Graphics));
            }
            else newGraphics = (Graphics *) malloc (sizeof (Graphics));

            Graphics *graphicsData = (Graphics *) data;
            newGraphics->objectId = item->itemId;
            newGraphics->name = graphicsData->name;
            newGraphics->glyph = graphicsData->glyph;
            newGraphics->fgColor = graphicsData->fgColor;
            newGraphics->bgColor = graphicsData->bgColor;

            item->components[type] = newGraphics;
            insertAfter (graphics, NULL, newGraphics);
        } break;
        default: break;
    }

}

void addItemComp (Item *item, ItemComponent type, void *data) {

    if (item == NULL || data == NULL) return;

    switch (type) {
        case WEAPON: {
            if (getItemComponent (item, type) != NULL) return;
            Weapon *newWeapon = (Weapon *) malloc (sizeof (Weapon));
            Weapon *weaponData = (Weapon *) data;
            newWeapon->itemId = item->itemId;
            newWeapon->dbId = item->dbId;
            newWeapon->dps = weaponData->dps;
            newWeapon->maxLifetime = weaponData->maxLifetime;
            newWeapon->lifetime = weaponData->lifetime;
            newWeapon->isEquipped = weaponData->isEquipped;
            item->itemComps[type] = newWeapon;
            insertAfter (weapons, NULL, newWeapon);
        } break;
        case ARMOUR: {
            if (getItemComponent (item, type) != NULL) return;
            Armour *newArmour = (Armour *) malloc (sizeof (Armour));
            Armour *armourData = (Armour *) data;
            newArmour->itemId = item->itemId;
            newArmour->dbId = item->dbId;
            newArmour->maxLifetime = armourData->maxLifetime;
            newArmour->lifetime = armourData->lifetime;
            newArmour->slot = armourData->slot;
            newArmour->isEquipped = armourData->isEquipped;
            item->itemComps[type] = newArmour;
            insertAfter (equipment, NULL, newArmour);
        } break;
        default: break;
    }

}
 
void removeGameComponent (Item *item, GameComponent type) {

    if (item == NULL) return;

    switch (type) {
        case POSITION: {
            Position *posComp = (Position *) getGameComponent (item, type);
            if (posComp != NULL) {
                ListElement *e = getListElement (positions, posComp);
            if (e != NULL) push (posPool, removeElement (positions, e));
            item->components[type] = NULL;
            }
        } break;
        case GRAPHICS: {
            Graphics *graComp = (Graphics *) getGameComponent (item, type);
            if (graComp == NULL) return;
            ListElement *e = getListElement (graphics, graComp);
            if (e != NULL) push (graphicsPool, removeElement (graphics, e));
            item->components[type] = NULL;
        } break;
        default: break;
    }

}

void removeItemComponent (Item *item, ItemComponent type) {

    if (item == NULL) return;

    switch (type) {
        case WEAPON: {
            Weapon *weapon = (Weapon *) getItemComponent (item , type);
            if (weapon == NULL) return;
            ListElement *e = getListElement (weapons, weapon);
            if (e != NULL) {
                void *weaponData = removeElement (weapons, e);
                free (weaponData);
                item->itemComps[type] = NULL;
            }
        } break;
        case ARMOUR: {
            Armour *armour = (Armour *) getItemComponent (item, type);
            if (armour == NULL) return;
            ListElement *e = getListElement (equipment, armour);
            if (e != NULL) {
                void *armourData = removeElement (equipment, e);
                free (armourData);
                item->itemComps[type] = NULL;
            }
        } break;
        default: break;
    }

}

void destroyItem (Item *item) {

    for (u8 i = 0; i < GAME_OBJECT_COMPS; i++) removeGameComponent (item, i);
    for (u8 i = 0; i < ITEM_COMPS; i++) removeItemComponent (item, i);

    ListElement *e = getListElement (items, item);
    if (e != NULL) push (itemsPool, removeElement (items, e));

}

void removeFromInventory (void *i) {

    for (ListElement *e = LIST_START (playerComp->inventory); e != NULL; e = e->next) {
        if (i == e->data) {
            destroyItem ((Item *) removeElement (playerComp->inventory, e));
            break;
        }
    }

}

// 28/08/2018 -- 11:15 -- testing effects inside items
void healPlayer (void *i) {

    Item *item = (Item *) i;
    Combat *playerCombat = (Combat *) getComponent (player, COMBAT);
    if (playerCombat != NULL) {
        i32 *currHealth = &playerCombat->baseStats.health;
        u32 maxHealth = playerCombat->baseStats.maxHealth;

        // FIXME: get the real data
        u16 health = 5;

        if (*currHealth == maxHealth) 
            logMessage ("You already have full health.", WARNING_COLOR);

        else {
            u16 realHp;

            *currHealth += health;

            // clamp the value if necessary
            if (*currHealth > maxHealth) {
                realHp = health - (*currHealth - maxHealth);
                *currHealth = maxHealth;
            }

            else realHp = health;  

            item->quantity--;
            if (item->quantity == 0) removeFromInventory (item);

            // TODO: maybe better strings
            // you have ate the apple for 5 health
            char *str = createString ("You have been healed by %i hp.", realHp);
            logMessage (str, SUCCESS_COLOR);
            free (str);
        }
        
    }

}

// 29/08/2018 -- 23:34 -- new way of creating an item using sqlite db
Item *createItem (u16 itemId) {

    sqlite3_stmt *res;
    char *sql = "SELECT Id, Name, Glyph, Rarity, Stackable, Quantity, Gold, Silver, Copper FROM Food WHERE Id = ?";
    int rc = sqlite3_prepare_v2 (itemsDb, sql, -1, &res, 0);

    if (rc == SQLITE_OK) sqlite3_bind_int (res, 1, itemId);
    else fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (itemsDb));

    int step = sqlite3_step (res);

    if (step == SQLITE_ROW) {
        fprintf (stdout, "%i: ", sqlite3_column_int (res, itemId));
        fprintf(stdout, "%s\n", sqlite3_column_text (res, NAME_COL));
    }

    Item *item = newItem ();
    asciiChar glyph = (asciiChar) sqlite3_column_int (res, GLYPH_COL);
    const char *temp = sqlite3_column_text (res, NAME_COL);
    char *name = (char *) calloc (strlen (temp), sizeof (char));
    strcpy (name, temp);

    // FIXME: COLOR
    u32 color = 0xFFFFFFFF;
    Graphics g = { 0, glyph, color, 0x000000FF, false, false, name };
    addGameComponent (item, GRAPHICS, &g);

    item->dbId = itemId;
    item->rarity = sqlite3_column_int (res, 3);
    item->stackable = (sqlite3_column_int (res, STACKABLE_COL) == 0) ? false : true;
    item->quantity = sqlite3_column_int (res, QUANTITY_COL);
    item->value[0] = (u16) sqlite3_column_int (res, GOLD_COL);
    item->value[1] = (u16) sqlite3_column_int (res, SILVER_COL);
    item->value[2] = (u16) sqlite3_column_int (res, COPPER_COL);

    sqlite3_finalize(res);

    return item;

} 
 
// Old way of creating an item using a cfg file
/* Item *createItem (u16 itemId) {

    ConfigEntity *itemEntity = getEntityWithId (itemsConfig, itemId);
    if (itemEntity == NULL) return NULL;

    Item *item = newItem ();

    asciiChar glyph = atoi (getEntityValue (itemEntity, "glyph"));
    char *name = getEntityValue (itemEntity, "name");
    u32 color = xtoi (getEntityValue (itemEntity, "color"));
    Graphics g = { 0, glyph, color, 0x000000FF, false, false, name };
    addGameComponent (item, GRAPHICS, &g);

    // 28/08/2018 -- 11:15 -- testing effects inside items
    u8 health = atoi (getEntityValue (itemEntity, "health"));
    if (health != 0) item->callback = healPlayer;

    item->dbId = itemId;
    item->type = atoi (getEntityValue (itemEntity, "type"));
    item->rarity = atoi (getEntityValue (itemEntity, "rarity"));
    item->stackable = atoi (getEntityValue (itemEntity, "stackable"));
    item->quantity = atoi (getEntityValue (itemEntity, "quantity"));
    item->weight = atoi (getEntityValue (itemEntity, "weight"));
    item->value[0] = atoi (getEntityValue (itemEntity, "gold"));
    item->value[1] = atoi (getEntityValue (itemEntity, "silver"));
    item->value[2] = atoi (getEntityValue (itemEntity, "copper"));
        
    return item;

} */

// check how much the player is carrying in its inventory and equipment
u16 getCarriedWeight (void) {

    u16 weight = 0;
    Item *item = NULL;
    for (ListElement *e = LIST_START (playerComp->inventory); e != NULL; e = e->next) {
        item = (Item *) LIST_DATA (e);
        if (item != NULL) weight += (item->weight * item->quantity);
    }

    return weight;

}

List *getItemsAtPos (u8 x, u8 y) {

    Position *pos = NULL;
    if (items == NULL || (LIST_SIZE (items) == 0)) return NULL;

    List *retVal = initList (free);
    for (ListElement *e = LIST_START (items); e != NULL; e = e->next) {
        pos = (Position *) getGameComponent ((Item *) e->data, POSITION);
        // inventory items do NOT have a pos comp
        if (pos != NULL) {
            if (pos->x == x && pos->y == y) insertAfter (retVal, NULL, e->data);
        }
    }

    return retVal;

}

u32 getItemColor (u8 rarity) {

    u32 color;
    switch (rarity) {
        case 0: color = RUBISH_COLOR; break;
        case 1: color = COMMON_COLOR; break;
        case 2: color = RARE_COLOR; break;
        case 3: color = EPIC_COLOR; break;
        case 4: color = LEGENDARY_COLOR; break;
        default: color = COMMON_COLOR; break;
    }

    return color;

}

bool itemStacked (Item *item) {

    Item *invItem = NULL;
    bool stacked = false;
    for (ListElement *e = LIST_START (playerComp->inventory); e != NULL; e = e->next) {
        invItem = (Item *) e->data;
        if (invItem->dbId == item->dbId) {
            if (invItem->quantity < MAX_STACK) {
                invItem->quantity += 1;
                destroyItem (item);
                stacked = true;
                break;
            }
            
            else continue;
        }
    }

    return stacked;

}

// pickup the first item of the list
void pickUp (Item *item) {

    if (item != NULL) {
        if ((getCarriedWeight () + item->weight) <= playerComp->maxWeight) {
            // add the item to the inventory
            if (item->stackable && (LIST_SIZE (playerComp->inventory) > 0)) {
                if (!itemStacked (item)) 
                    insertAfter (playerComp->inventory, LIST_END (playerComp->inventory), item);
            }

            else insertAfter (playerComp->inventory, LIST_END (playerComp->inventory), item);
            
            // remove the item from the map
            removeGameComponent (item, POSITION);

            Graphics *g = (Graphics *) getGameComponent (item, GRAPHICS);
            if (g != NULL) {
                if (g->name != NULL) logMessage (createString ("You picked up the %s.", g->name), DEFAULT_COLOR);
                else logMessage ("Picked up the item!", DEFAULT_COLOR);
            }

            playerTookTurn = true;
        }

        else logMessage ("You are carrying to much already!", WARNING_COLOR);
    }

}


// As of 16/08/2018:
// The character must be on the same coord as the item to be able to pick it up
void getItem (void) {

    fprintf (stdout, "Getting item...\n");

    Position *playerPos = (Position *) getComponent (player, POSITION);
    // get a list of items nearby the player
    List *objects = getItemsAtPos (playerPos->x, playerPos->y);

    fprintf (stdout, "Got a list of objects\n");

    if (objects == NULL || (LIST_SIZE (objects) <= 0)) {
        fprintf (stdout, "Lis is empty!\n");
        if (objects != NULL) free (objects);
        logMessage ("There are no items here!", WARNING_COLOR);
        
    }

    // we only pick one item each time
    else {
        pickUp ((Item *)((LIST_START (objects))->data));
        fprintf (stdout, "Objects at pos: %i\n", LIST_SIZE (objects));
        if (objects != NULL) destroyList (objects);
    } 

}

extern Loot *currentLoot;

void getLootItem (u8 lootYIdx) {

    if (currentLoot != NULL) {
        // we only pick one item each time
        if ((currentLoot->lootItems != NULL) && (LIST_SIZE (currentLoot->lootItems) > 0)) {
            Item *item = NULL;
            u8 count = 0;
            for (ListElement *e = LIST_START (currentLoot->lootItems); e != NULL; e = e->next) {
                if (count == lootYIdx) {
                    item = (Item *) removeElement (currentLoot->lootItems, e);
                    break;
                }

                count++;
            }

            pickUp (item);

            // update Loot UI
            updateLootUI (lootYIdx);
        }

        else logMessage ("There are no items to pick up!", WARNING_COLOR);
    }

    else logMessage ("There is not loot available here!", WARNING_COLOR);

}

void dropItem (Item *item) {

    if (item == NULL) return;
    if (item->quantity <= 0) return;    // quick dirty fix

    Item *dropItem = NULL;

    if (item->stackable) {
        item->quantity--;
        if (item->quantity > 0) dropItem = createItem (item->dbId);

        else {
            for (ListElement *e = LIST_START (playerComp->inventory); e != NULL; e = e->next) 
                if (e->data == (void *) item) 
                    dropItem = (Item *) removeElement (playerComp->inventory, e);
        } 

    }

    else {
        for (ListElement *e = LIST_START (playerComp->inventory); e != NULL; e = e->next) 
            if (e->data == (void *) item) 
                dropItem = (Item *) removeElement (playerComp->inventory, e);

    }

    Position *playerPos = (Position *) getComponent (player, POSITION);
    Position pos = { .x = playerPos->x, .y = playerPos->y, .layer = MID_LAYER };
    addGameComponent (dropItem, POSITION, &pos);

    Graphics *g = (Graphics *) getGameComponent (dropItem, GRAPHICS);
    if (g != NULL) {
        char *msg = createString ("You dropped the %s.", g->name);
        logMessage (msg, DEFAULT_COLOR);
        free (msg);
    }
    else logMessage ("You dropped the item.", DEFAULT_COLOR);

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

        // TODO: update the player stats based on the new item
        // Combat *itemCombat = (Combat *) getComponent (item, COMBAT);
        // Combat *playerCombat = (Combat *) getComponent (player, COMBAT);
        
    }

}

// TODO: crafting

// TODO: I think we will want to add the ability to repair your items in a shop,
// but only if they are above 0, if you don't repair your items soon enough, 
// you will lose them
void repairItems (void) {

}

// FIXME:
// only reduce lifetime of weapons, and equipment
void updateLifeTime (void) {

    GameObject *go = NULL;
    Item *item = NULL;
    for (ListElement *e = LIST_START (playerComp->inventory); e != NULL; e = e->next) {
        go = (GameObject *) LIST_DATA (e);
        item = (Item *) getComponent (go, ITEM);
        // FIXME: only for weapons and armor

        // FIXME:
        // if (item->lifetime <= 0) {
        //     bool wasEquipped = false;
        //     if (item->isEquipped) {
        //         wasEquipped = true;
        //         // FIXME: unequip item
        //     }

        //     // remove from inventory
        //     // FIXME:

        //     // TODO: give feedback to the player with messages in the log
        // }
    }

}

/*** CLEAN UP ***/

void cleanUpItems (void) {

    destroyList (items);
    clearPool (itemsPool);

    // clearConfig (itemsConfig);

    // disconnect from the db
    sqlite3_close (itemsDb);

    fprintf (stdout, "Done cleaning up items.\n");

}
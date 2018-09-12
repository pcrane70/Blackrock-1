#include <sqlite3.h>
#include <string.h>

#include "blackrock.h"
#include "game.h"
#include "item.h"
#include "player.h"

#include "utils/list.h"
#include "objectPool.h"

#include "ui/gameUI.h" // for strings

#include "utils/myUtils.h"


// our items db
const char *itemsDbPath = "./data/items.db";    // The path is form the makefile
sqlite3 *itemsDb;

List *items = NULL;
Pool *itemsPool = NULL;

// static u32 itemsId = 0;

extern unsigned int newId;

extern void die (char *);

// FIXME: problems with items pool
void initItems (void) {

    items = initList (free);
    itemsPool = initPool ();
    if (itemsPool = NULL) fprintf (stderr, "Items pool is NULL!\n");

    // connect to the items db
    if (sqlite3_open (itemsDbPath, &itemsDb) != SQLITE_OK) {
        fprintf (stderr, "%s\n", sqlite3_errmsg (itemsDb));
        die ("Problems with items db!\n");
    } 

    // development functions
    // This are used only for testing and to populate our dbs
    void createItemsDb (void);
    // createItemsDb ();

}

/*** ITEMS IN MEMORY ***/

// FIXME: problems with items pool!!
Item *newItem (void) {

    Item *i = NULL;

    // check if there is a an available one in the items pool
    // if (POOL_SIZE (itemsPool) > 0) {
    //     i = pop (itemsPool);
    //     if (i == NULL) i = (Item *) malloc (sizeof (Item));
    // } 
    // else i = (Item *) malloc (sizeof (Item));

    i = (Item *) malloc (sizeof (Item));

    if (i != NULL) {
        i->itemId = newId;
        newId++;
        for (u8 u = 0; u < GAME_OBJECT_COMPS; u++) i->components[u] = NULL;
        for (u8 u = 0; u < ITEM_COMPS; u++) i->itemComps[u] = NULL;
        insertAfter (items, NULL, i);
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
            newGraphics->name = (char *) calloc (strlen (graphicsData->name) + 1, sizeof (char));
            strcpy (newGraphics->name, graphicsData->name);
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
            newWeapon->type = weaponData->type;
            newWeapon->dps = weaponData->dps;
            newWeapon->maxLifetime = weaponData->maxLifetime;
            newWeapon->lifetime = weaponData->lifetime;
            newWeapon->isEquipped = weaponData->isEquipped;
            newWeapon->slot = weaponData->slot;
            item->itemComps[type] = newWeapon;
        } break;
        case ARMOUR: {
            if (getItemComponent (item, type) != NULL) return;
            Armour *newArmour = (Armour *) malloc (sizeof (Armour));
            Armour *armourData = (Armour *) data;
            newArmour->itemId = item->itemId;
            newArmour->dbId = item->dbId;
            newArmour->type = armourData->type;
            newArmour->maxLifetime = armourData->maxLifetime;
            newArmour->lifetime = armourData->lifetime;
            newArmour->slot = armourData->slot;
            newArmour->isEquipped = armourData->isEquipped;
            item->itemComps[type] = newArmour;
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
            if (weapon != NULL) {
                item->itemComps[type] = NULL;
                free (weapon);
            }
        } break;
        case ARMOUR: {
            Armour *armour = (Armour *) getItemComponent (item, type);
            if (armour != NULL) {
                item->itemComps[type] = NULL;
                free (armour);
            }
        } break;
        default: break;
    }

}

// FIXME: problems with items pool
Item *destroyItem (Item *item) {

    for (u8 i = 0; i < GAME_OBJECT_COMPS; i++) removeGameComponent (item, i);
    for (u8 i = 0; i < ITEM_COMPS; i++) removeItemComponent (item, i);

    ListElement *e = getListElement (items, item);
    void *data = removeElement (items, e);
    if (data != NULL) free (data);

    // if (e != NULL) push (itemsPool, removeElement (items, e));

    return NULL;

}

Item *deleteItem (Item *item) {

    for (u8 i = 0; i < GAME_OBJECT_COMPS; i++) removeGameComponent (item, i);
    for (u8 i = 0; i < ITEM_COMPS; i++) removeItemComponent (item, i);

    ListElement *e = getListElement (items, item);
    if (e != NULL) {
        void *old = removeElement (items, e);
        if (old != NULL) free (old);
    }

    return NULL;

}

// search for the item in the inventory and remove it
Item *removeFromInventory (Item *item) {

    Item *retVal = NULL;
    bool removed = false;

    for (u8 y = 0; y < 3; y++) {
        for (u8 x = 0; x < 7; x++) {
            if (!removed) {
                if (item == player->inventory[x][y]) {
                    retVal = player->inventory[x][y];
                    player->inventory[x][y] = NULL;
                    removed = true;
                }
            }
            
        }
    }

    return retVal;

}

/*** CREATING ITEMS ***/

u8 addGraphicsToItem (u32 itemId, Item *item, char *itemName) {

    // get the db data
    sqlite3_stmt *res;
    char *sql = "SELECT * FROM Graphics WHERE Id = ?";

    if (sqlite3_prepare_v2 (itemsDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, itemId);
    else {
        fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (itemsDb));
        return 1;
    } 

    int step = sqlite3_step (res);

    asciiChar glyph = (asciiChar) sqlite3_column_int (res, 1);
    const char *c = sqlite3_column_text (res, 2);
    char *colour = (char *) calloc (strlen (c) + 1, sizeof (char));
    strcpy (colour, c);
    u32 color = (u32) xtoi (colour);
    Graphics g = { 0, glyph, color, 0x000000FF, false, false, itemName };
    addGameComponent (item, GRAPHICS, &g);

    free (colour);
    sqlite3_finalize (res);

    return 0;

}

void healPlayer (void *);
void toggleEquipWeapon (void *);
void toggleEquipArmour (void *);

EventListener getItemCallback (u8 cb) {

    EventListener callback = NULL;

    switch (cb) {
        case 0: callback = NULL; break;
        case 1: callback = healPlayer; break;
        case 2: callback = toggleEquipWeapon; break;
        case 3: callback = toggleEquipArmour; break;
        default: break;
    }

    return callback;

}

// 29/08/2018 -- 23:34 -- new way of creating an item using sqlite db
// 05/09/2018 -- 11:04 -- creating items with a more complex db
Item *createItem (int itemId) {

    // get the db data
    sqlite3_stmt *res;
    char *sql = "SELECT * FROM Items WHERE Id = ?";

    if (sqlite3_prepare_v2 (itemsDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, itemId);
    else {
        fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (itemsDb));
        return NULL;
    } 

    int step = sqlite3_step (res);

    Item *item = newItem ();
    if (item == NULL) return NULL;

    item->dbId = itemId;

    char name[50];
    const char *itemName = sqlite3_column_text (res, ITEM_NAME_COL);
    strcpy (name, itemName);
    item->rarity = (u8) sqlite3_column_int (res, ITEM_RARITRY_COL);
    item->value[0] = sqlite3_column_int (res, ITEM_GOLD_COL);
    item->value[1] = sqlite3_column_int (res, ITEM_SILVER_COL);
    item->value[2] = sqlite3_column_int (res, ITEM_COPPER_COL);
    item->probability = sqlite3_column_double (res, ITEM_PROB_COL);
    item->stackable = (sqlite3_column_int (res, ITEM_STACKABLE_COL) == 0) ? false : true;
    item->quantity = (u8) sqlite3_column_int (res, ITEM_QUANTITY_COL);
    
    item->callback = getItemCallback ((u8) sqlite3_column_int (res, ITEM_CALLBACK_COL));

    // graphics
    if (addGraphicsToItem (itemId, item, name) != 0) {
        fprintf (stderr, "Error adding graphics component!\n");
        return NULL;
    } 

    sqlite3_finalize (res);

    return item;

} 

Item *createWeapon (u32 itemId) {

    Item *weapon = createItem (itemId);

    weapon->stackable = false;
    weapon->quantity = 1;

    // get the db data
    sqlite3_stmt *res;
    char *sql = "SELECT * FROM Weapons WHERE Id = ?";

    if (sqlite3_prepare_v2 (itemsDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, itemId);
    else {
        fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (itemsDb));
        return NULL;
    } 

    int step = sqlite3_step (res);

    Weapon w;
    w.dbId = itemId;
    w.type = (u8) sqlite3_column_int (res, 1);
    w.twoHanded = (sqlite3_column_int (res, 2) == 0) ? false : true;
    w.dps = (u8) sqlite3_column_int (res, 3);
    w.slot = sqlite3_column_int (res, 4);
    w.maxLifetime = (u16) sqlite3_column_int (res, 5);
    w.lifetime = w.maxLifetime;
    w.isEquipped = false;
    addItemComp (weapon, WEAPON, &w);

    sqlite3_finalize (res);

    return weapon;

}

Item *createArmour (u32 itemId) {

    Item *armour = createItem (itemId);

    armour->stackable = false;
    armour->quantity = 1;

    // get the db data
    sqlite3_stmt *res;
    char *sql = "SELECT * FROM Armour WHERE Id = ?";

    if (sqlite3_prepare_v2 (itemsDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, itemId);
    else {
        fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (itemsDb));
        return NULL;
    } 

    int step = sqlite3_step (res);

    Armour a;
    a.dbId = itemId;
    a.type = (u8) sqlite3_column_int (res, 1);
    a.slot = (u8) sqlite3_column_int (res, 2);
    a.maxLifetime = (u16) sqlite3_column_int (res, 3);
    a.lifetime = a.maxLifetime;
    a.isEquipped = false;
    addItemComp (armour, WEAPON, &a);

    sqlite3_finalize (res);

    return armour;

}

/*** ITEMS FUNCS ***/

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

char *getItemSlot (Item *item) {

    char slot[15];

    Weapon *w = getItemComponent (item, WEAPON);
    if (w != NULL) {
        switch (w->slot) {
            case 0: strcpy (slot, "Main"); break;
            case 1: strcpy (slot, "Off"); break;
            default: break;
        }
    }

    else {
        Armour *a = getItemComponent (item, ARMOUR);
        if (a != NULL) {
            switch (a->slot) {
                case 0: strcpy (slot, "Head"); break;
                case 1: strcpy (slot, "Necklace"); break;
                case 2: strcpy (slot, "Shoulders"); break;
                case 3: strcpy (slot, "Cape"); break;
                case 4: strcpy (slot, "Chest"); break;
                case 5: strcpy (slot, "Hands"); break;
                case 6: strcpy (slot, "Waist"); break;
                case 7: strcpy (slot, "Legs"); break;
                case 8: strcpy (slot, "Shoes"); break;
                case 9: strcpy (slot, "Ring"); break;
                default: break;
            }
        }
    }

    char *retVal = (char *) calloc (strlen (slot), sizeof (char));
    strcpy (retVal, slot);

    return retVal;

}

bool itemStacked (Item *item) {

    bool stacked = false;

    for (u8 y = 0; y < 3; y++) {
        for (u8 x = 0; x < 7; x++) {
            if (player->inventory[x][y] != NULL && !stacked) {
                if (player->inventory[x][y]->dbId == item->dbId) {
                    if (player->inventory[x][y]->quantity < MAX_STACK) {
                        player->inventory[x][y]->quantity += 1;
                        destroyItem (item);
                        stacked = true;
                    }
                }
            }
        }
    }

    return stacked;

}

void addToInventory (Item *item) {

    bool inserted = false;

    if (item->stackable && inventoryItems > 0) {
        // insert in the next available inventory slot
        if (!itemStacked (item)) {
            for (u8 y = 0; y < 3; y++) {
                for (u8 x = 0; x < 7; x++) {
                    if (player->inventory[x][y] == NULL && !inserted) {
                        player->inventory[x][y] = item;
                        inserted = true;
                        inventoryItems += 1;
                    }
                }
            }
        }
    }

    // insert in the next available inventory slot
    else {
        for (u8 y = 0; y < 3; y++) {
            for (u8 x = 0; x < 7; x++) {
                if (player->inventory[x][y] == NULL && !inserted) {
                    player->inventory[x][y] = item;
                    inserted = true;
                    inventoryItems += 1;
                }
            }
        }
    } 

}

// pickup the first item of the list
void pickUp (Item *item) {

    if (item != NULL) {
        addToInventory (item);
            
        // remove the item from the map
        removeGameComponent (item, POSITION);

        Graphics *g = (Graphics *) getGameComponent (item, GRAPHICS);
        if (g != NULL) {
            if (g->name != NULL) logMessage (createString ("You picked up the %s.", g->name), DEFAULT_COLOR);
            else logMessage ("Picked up the item!", DEFAULT_COLOR);
        }

        playerTookTurn = true;
    }

}

// As of 16/08/2018:
// The character must be on the same coord as the item to be able to pick it up
void getItem (void) {

    // get a list of items nearby the player
    List *objects = getItemsAtPos (player->pos->x, player->pos->y);

    if (objects == NULL || (LIST_SIZE (objects) <= 0)) {
        if (objects != NULL) free (objects);
        logMessage ("There are no items here!", WARNING_COLOR);
        
    }

    // we only pick one item each time
    else {
        pickUp ((Item *) removeElement (objects, (LIST_START (objects))));
        if (objects != NULL) cleanUpList (objects);
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

// TODO: add the option to drop the entiere stack
void dropItem (Item *item) {

    if (item == NULL) return;
    if (item->quantity <= 0) return;    // quick dirty fix

    Item *dropItem = NULL;

    if (item->stackable) {
        if (item->quantity > 1) {
            item->quantity--;
            dropItem = createItem (item->dbId);
        } 

        else dropItem = removeFromInventory (item);
    }

    else dropItem = removeFromInventory (item);

    inventoryItems--;

    // update the UI
    resetInventoryRects ();

    Position pos = { .x = player->pos->x, .y = player->pos->y, .layer = MID_LAYER };
    addGameComponent (dropItem, POSITION, &pos);

    Graphics *g = (Graphics *) getGameComponent (dropItem, GRAPHICS);
    if (g != NULL) {
        char *msg = createString ("You dropped the %s.", g->name);
        logMessage (msg, DEFAULT_COLOR);
        free (msg);
    }

    else logMessage ("You dropped the item.", DEFAULT_COLOR);

}

/*** WEAPONS -- ARMOUR ***/

// TODO: check for specific class weapons
// TODO: update combat stats based on weapon modifiers if necessary
void toggleEquipWeapon (void *i) {

    if (i == NULL) return;

    Item *item = (Item *) i;
    Weapon *weapon = (Weapon *) getItemComponent (item, WEAPON);
    if (weapon == NULL) {
        fprintf (stderr, "No weapon component!!\n");
        return;
    }

    // unequip
    if (weapon->isEquipped) {
        Item *w = player->weapons[weapon->slot];
        if (w != NULL) {
            addToInventory (w);

            Graphics *g = (Graphics *) getGameComponent (w, GRAPHICS);
            if (g != NULL) {
                char *str = createString ("You unequip the %s", g->name);
                logMessage (str, DEFAULT_COLOR);
                free (str);
            } 

            player->weapons[weapon->slot] = NULL;

            weapon->isEquipped = false;
        }
    }

    // equip
    else {
        Item *w = removeFromInventory (item);
        if (w != NULL) {
            // unequip our current weapon if we have one
            if (player->weapons[weapon->slot] != NULL) 
                toggleEquipWeapon (player->weapons[weapon->slot]);

            // if we are equipping a two handed and we have two one handed
            if (((Weapon *) getItemComponent (w, WEAPON))->twoHanded) {
                if (player->weapons[1] != NULL)
                    toggleEquipWeapon (player->weapons[1]); // unequip the off hand weapon

            }

            player->weapons[weapon->slot] = w;
            Graphics *g = (Graphics *) getGameComponent (w, GRAPHICS);
            if (g != NULL) {
                char *str = createString ("You are now wielding the %s", g->name);
                logMessage (str, DEFAULT_COLOR);
                free (str);
            }

            weapon->isEquipped = true;
        }
    }

    // update the UI
    resetCharacterRects ();
    resetInventoryRects ();
        
}

// TODO: update combat stats based on armour modifiers if necessary
// TODO: create better strings deppending on the item
void toggleEquipArmour (void *i) {

    if (i == NULL) return;

    Item *item = (Item *) i;
    Armour *armour = (Armour *) getItemComponent (item, ARMOUR);
    if (armour == NULL) {
        fprintf (stderr, "No armour component!!\n");
        return;
    }

    // unequip
    if (armour->isEquipped) {
        Item *a = player->equipment[armour->slot];
        if (a != NULL) {
            addToInventory (a);
            Graphics *g = (Graphics *) getGameComponent (a, GRAPHICS);
            if (g != NULL) {
                char *str = createString ("You take off the %s", g->name);
                logMessage (str, DEFAULT_COLOR);
                free (str);
            } 

            player->equipment[armour->slot] = NULL;

            armour->isEquipped = false;
        }
    }

    // equip
    else {
        Item *a = removeFromInventory (item);
        if (a != NULL) {
            // unequip the armour in that slot if we have one
            if (player->equipment[armour->slot] != NULL)
                toggleEquipArmour (player->equipment[armour->slot]);

            player->equipment[armour->slot] = a;
            Graphics *g = (Graphics *) getGameComponent (a, GRAPHICS);
            if (g != NULL) {
                char *str = createString ("Yoou are now wielding the %s", g->name);
                logMessage (str, DEFAULT_COLOR);
                free (str);
            }

            armour->isEquipped = true;
        }
    }

    // update the UI
    resetCharacterRects ();
    resetInventoryRects ();

}

// TODO: crafting

// TODO: I think we will want to add the ability to repair your items in a shop,
// but only if they are above 0, if you don't repair your items soon enough, 
// you will lose them
void repairItems (void) {

}

// As of 30/08/2018 -- 11:10 -- if an item get to lifetime = 0, it will unequip and 
// go to the inventory... if there is no space in the inventory, it will stay equipped
// but it will loose all of its qualities

// FIXME:
// only reduce lifetime of weapons, and equipment
void updateLifeTime (void) {

    Item *item = NULL;

    // weapons 
    Weapon *weapon = NULL;
    for (u8 i = 0; i < 3; i++) {
        item = player->weapons[i];
        weapon = (Weapon *) getItemComponent (item, WEAPON);

        // FIXME: update lifetime of weapons when hitting a mob

        if (weapon->lifetime <= 0) {
            // unequip
            // toggleEquipWeapon (item);
        }
    }

    // TODO:
    // armour
    // Armour *armour = NULL;
    // for (u8 i = 0; i < 9; i++) {
    //     item = playerComp->equipment[i];
    //     armour = (Armour *) getItemComponent (item, ARMOUR);

    //     // FIXME: update lifetime of armour when being hit from a mob

    //     if (armour->lifetime <= 0) {
            
    //     }
    // }

}

/*** CALLBACKS ***/

void healPlayer (void *i) {

    Item *item = (Item *) i;

    i32 *currHealth = &player->combat->baseStats.health;
    u32 maxHealth = player->combat->baseStats.maxHealth;

    // FIXME: get the real data
    // FIXME: 09/09/2018 -- 23:08 -- this is just for testing
    u16 health = (u16) randomInt (5, 10);

    if (*currHealth == maxHealth) logMessage ("You already have full health.", WARNING_COLOR);
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
        if (item->quantity == 0) {
            Item *old = removeFromInventory (item);
            if (old != NULL) destroyItem (old);

            resetInventoryRects ();
        }

        // TODO: maybe better strings
        // you have ate the apple for 5 health
        char *str = createString ("You have been healed by %i hp.", realHp);
        logMessage (str, SUCCESS_COLOR);
        free (str);
    }

}

/*** CLEAN UP ***/

// FIXME: problems with items pool
void cleanUpItems (void) {

    destroyList (items);
    // clearPool (itemsPool);

    // disconnect from the db
    sqlite3_close (itemsDb);

}

/*** DEVELOPMENT - DATABASE ***/

// 05/09/2018 -- new way for creating our items db 
void createItemsDb (void) {

    char *err_msg = 0;

    // master item
    char *sql = "DROP TABLE IF EXISTS Items;"
                "CREATE TABLE Items(Id INT, Name TEXT, Rarity INT, Gold INT, Silver INT, Copper INT, Probability DOUBLE, Quantity INT, Stackable INT, Callback INT);";

    if (sqlite3_exec (itemsDb, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf (stderr, "Error! Failed create ITEMS table!\n");
        fprintf (stderr, "SQL error: %s\n", err_msg);
        sqlite3_free (err_msg);
    }

    // graphics
    sql = "DROP TABLE IF EXISTS Graphics;"
          "CREATE TABLE Graphics(Id INT, Glyph INT, Color TEXT)";

    if (sqlite3_exec (itemsDb, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf (stderr, "Error! Failed create GRAPHICS table!\n");
        fprintf (stderr, "SQL error: %s\n", err_msg);
        sqlite3_free (err_msg);
    }

    // weapons
    sql = "DROP TABLE IF EXISTS Weapons;"
          "CREATE TABLE Weapons(Id INT, Type INT, Two INT, Dps INT, Slot INT, Lifetime INT);";

    if (sqlite3_exec (itemsDb, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf (stderr, "Error! Failed create WEAPONS table!\n");
        fprintf (stderr, "SQL error: %s\n", err_msg);
        sqlite3_free (err_msg);
    }

    // armour
    sql = "DROP TABLE IF EXISTS Armour;"
          "CREATE TABLE Armour(Id INT, Type INT, Slot INT, Lifetime INT);";

    if (sqlite3_exec (itemsDb, sql, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf (stderr, "Error! Failed create ARMOUR table!\n");
        fprintf (stderr, "SQL error: %s\n", err_msg);
        sqlite3_free (err_msg);
    }

}
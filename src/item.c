#include <string.h>

#include "blackrock.h"
#include "game.h"
#include "player.h"

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

Item *deleteItem (Item *item) {

    for (u8 i = 0; i < GAME_OBJECT_COMPS; i++) removeGameComponent (item, i);
    for (u8 i = 0; i < ITEM_COMPS; i++) removeItemComponent (item, i);

    free (item);

    return NULL;

}

// FIXME: check for the destroy item function
void removeFromInventory (Item *i) {

    // search for the item in the inventory and remove it
    for (u8 y = 0; y < 3; y++) {
        for (u8 x = 0; x < 7; x++) {

        }
    }

}

// 28/08/2018 -- 11:15 -- testing effects inside items
void healPlayer (void *i) {

    Item *item = (Item *) i;

    i32 *currHealth = &player->combat->baseStats.health;
    u32 maxHealth = player->combat->baseStats.maxHealth;

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

// 29/08/2018 -- 23:34 -- new way of creating an item using sqlite db
Item *createItem (u16 itemId) {

    sqlite3_stmt *res;
    char *sql = "SELECT * FROM Food WHERE Id = ?";
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

// FIXME: check the destroy item function here!!
bool itemStacked (Item *item) {

    bool stacked = false;

    for (u8 y = 0; y < 3; y++) {
        for (u8 x = 0; x < 7; x++) {
            if (player->inventory[x][y] == NULL) continue;
            else if (player->inventory[x][y]->dbId == item->dbId) {
                if (player->inventory[x][y]->quantity < MAX_STACK) {
                    player->inventory[x][y]->quantity += 1;
                    destroyItem (item);
                    stacked = true;
                    break;
                }
            }
        }
    }

    return stacked;

}

void addToInventory (Item *item) {

    if (item->stackable && inventoryItems > 0) {
        // insert in the next available inventory slot
        if (!itemStacked (item)) {
            for (u8 y = 0; y < 3; y++) {
                for (u8 x = 0; x < 7; x++) {
                    if (player->inventory[x][y] != NULL) continue;
                    else {
                        player->inventory[x][y] = item;
                        break;
                    }
                }
            }
        }
    }

    // insert in the next available inventory slot
    else {
        for (u8 y = 0; y < 3; y++) {
            for (u8 x = 0; x < 7; x++) {
                if (player->inventory[x][y] != NULL) continue;
                else {
                    player->inventory[x][y] = item;
                    break;
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
        fprintf (stdout, "Lis is empty!\n");
        if (objects != NULL) free (objects);
        logMessage ("There are no items here!", WARNING_COLOR);
        
    }

    // we only pick one item each time
    else {
        pickUp ((Item *)((LIST_START (objects))->data));
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

// FIXME:
// void dropItem (Item *item) {

//     if (item == NULL) return;
//     if (item->quantity <= 0) return;    // quick dirty fix

//     Item *dropItem = NULL;

//     if (item->stackable) {
//         item->quantity--;
//         if (item->quantity > 0) dropItem = createItem (item->dbId);

//         else {
//             for (ListElement *e = LIST_START (playerComp->inventory); e != NULL; e = e->next) 
//                 if (e->data == (void *) item) 
//                     dropItem = (Item *) removeElement (playerComp->inventory, e);
//         } 

//     }

//     else {
//         for (ListElement *e = LIST_START (playerComp->inventory); e != NULL; e = e->next) 
//             if (e->data == (void *) item) 
//                 dropItem = (Item *) removeElement (playerComp->inventory, e);

//     }

//     Position *playerPos = (Position *) getComponent (player, POSITION);
//     Position pos = { .x = playerPos->x, .y = playerPos->y, .layer = MID_LAYER };
//     addGameComponent (dropItem, POSITION, &pos);

//     Graphics *g = (Graphics *) getGameComponent (dropItem, GRAPHICS);
//     if (g != NULL) {
//         char *msg = createString ("You dropped the %s.", g->name);
//         logMessage (msg, DEFAULT_COLOR);
//         free (msg);
//     }
//     else logMessage ("You dropped the item.", DEFAULT_COLOR);

// }

/*** WEAPONS -- EQUIPMENT ***/

// FIXME:
// TODO: check for specific class weapons
// TODO: update combat stats based on weapon modifiers if necessary
/* void toggleEquipWeapon (void *i) {

    if (i == NULL) return;

    Item *item = (Item *) i;
    Weapon *weapon = (Weapon *) getItemComponent (item, WEAPON);

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
        }
    }

    // equip
    else {
        ListElement *le = getListElement (player->inventory, i);
        Item *w = (Item *) removeElement (player->inventory, le);
        if (w != NULL) {
            // unequip our current weapon if we have one
            if (player->weapons[weapon->slot] != NULL) 
                toggleEquipWeapon (player->weapons[weapon->slot]);

            // if we are equipping a two handed and we have tow one handed
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
        }
    }
        
} */

// FIXME:
// TODO: update combat stats based on armour modifiers if necessary
// TODO: create better strings deppending on the item
/* void toggleEquipArmour (void *i) {

    if (i == NULL) return;

    Item *item = (Item *) i;
    Armour *armour = (Armour *) getItemComponent (item, ARMOUR);

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
        }
    }

    // equip
    else {
        ListElement *le = getListElement (player->inventory, i);
        Item *a = (Item *) removeElement (player->inventory, le);
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
        }
    }

} */

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

/*** CLEAN UP ***/

void cleanUpItems (void) {

    destroyList (items);
    clearPool (itemsPool);

    // clearConfig (itemsConfig);

    // disconnect from the db
    sqlite3_close (itemsDb);

    fprintf (stdout, "Done cleaning up items.\n");

}
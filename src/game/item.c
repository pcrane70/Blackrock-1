#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sqlite3.h>

#include "blackrock.h"

#include "game/game.h"
#include "game/item.h"

#include "engine/renderer.h"
#include "engine/sprites.h"

#include "utils/myUtils.h"
#include "utils/log.h"

#pragma region ITEMS

const char *itemsDbPath = "items.db";
static sqlite3 *itemsDb;

const char *accesoriesPath = "accesories-sprite-sheet.png";
const char *armorPath = "armor-sprite-sheet.png";
const char *foodPath = "food-sprite-sheet.png";
const char *miscPath = "misc-sprite-sheet";
const char *weaponsPath = "weapons-sprite-sheet";

SpriteSheet *accesories, *armor, *food, *misc, *weapons;

u8 items_connect_db (void) {

    if (sqlite3_open (createString ("%s%s", DATA_PATH, itemsDbPath), &itemsDb) != SQLITE_OK) {
        #ifdef DEV
        logMsg (stderr, ERROR, GAME, "Problems connecting to items db!");
        logMsg (stderr, ERROR, GAME, createString ("%s", sqlite3_errmsg (itemsDb)));
        #elif PRODUCTION
        logMsg (stderr, ERROR, NO_TYPE, "Failed to load game data!");
        #endif
        return 1;
    } 

    else {
        #ifdef DEV
        logMsg (stdout, DEBUG_MSG, GAME, "Connected to enemies db.");
        #endif
        return 1;
    }

}

// FIXME: set scale factor for sprite sheets
// TODO: how to handle a missing sprite sheet?
// load items sprite sheets
static u8 items_load_sprite_sheets (void) {

    u8 retval = 0;

    accesories = sprite_sheet_load (createString ("%sitems/%s", ASSETS_PATH, accesoriesPath), main_renderer);
    armor = sprite_sheet_load (createString ("%sitems/%s", ASSETS_PATH, armorPath), main_renderer);
    food = sprite_sheet_load (createString ("%sitems/%s", ASSETS_PATH, foodPath), main_renderer);
    misc = sprite_sheet_load (createString ("%sitems/%s", ASSETS_PATH, miscPath), main_renderer);
    weapons = sprite_sheet_load (createString ("%sitems/%s", ASSETS_PATH, weaponsPath), main_renderer);

    // crop the sprite sheets
    if (accesories && armor && food && misc && weapons) {
        // accesories
        sprite_sheet_set_sprite_size (accesories, ITEMS_PIXEL_SIZE, ITEMS_PIXEL_SIZE);
        sprite_sheet_crop (accesories);

        // armor
        sprite_sheet_set_sprite_size (armor, ITEMS_PIXEL_SIZE, ITEMS_PIXEL_SIZE);
        sprite_sheet_crop (armor);
        
        // food
        sprite_sheet_set_sprite_size (food, ITEMS_PIXEL_SIZE, ITEMS_PIXEL_SIZE);
        sprite_sheet_crop (food);

        // misc
        sprite_sheet_set_sprite_size (misc, ITEMS_PIXEL_SIZE, ITEMS_PIXEL_SIZE);
        sprite_sheet_crop (misc);

        // weapons
        sprite_sheet_set_sprite_size (weapons, ITEMS_PIXEL_SIZE, ITEMS_PIXEL_SIZE);
        sprite_sheet_crop (weapons);
    }  

    else retval = 1; 

    return retval;

}

u8 items_init (void) {

    // load item sprite sheets && connect to item db
    if (!items_load_sprite_sheets () && !items_connect_db ())
        return 0;

    return 1; 

}

void items_end (void) {

    // close items db
    sqlite3_close (itemsDb);

    // delete items sprites sheets
    sprite_sheet_destroy (accesories);
    sprite_sheet_destroy (armor);
    sprite_sheet_destroy (food);
    sprite_sheet_destroy (misc);
    sprite_sheet_destroy (weapons);

} 

#pragma endregion

#pragma region ITEM GO COMPONENT

Item *item_create_comp (u32 goID) {

    Item *item = (Item *) malloc (sizeof (Item));
    if (item) {
        memset (item, 0, sizeof (Item));
        item->goID = goID;
    }

    return item;

}

static void weapon_destroy (Weapon *weapon);
static void armour_destroy (Armour *armor);

void item_destroy_comp (Item *item) {

    if (item) {
        weapon_destroy (item->components[WEAPON_COMP]);
        armour_destroy (item->components[ARMOUR_COMP]);

        free (item);
    }

}

Item *item_add_item_comp (GameObject *item_go, u32 dbID) {

    if (item_go) {
        Item *item = game_object_add_component (item_go, ITEM_COMP);
        if (item) {
            item->dbID = dbID;
            // FIXME: load info from the db
        }
    }

}

#pragma endregion

#pragma region ITEM COMPONENTS

static Weapon *weapon_new (u32 dbID) {

    Weapon *weapon = (Weapon *) malloc (sizeof (Weapon));
    if (weapon) {
        // get weapon values from db
        sqlite3_stmt *res;
        char *sql = "SELECT * FROM Weapons WHERE Id = ?";

        if (sqlite3_prepare_v2 (itemsDb, sql, -1, &res, 0) == SQLITE_OK) {
            sqlite3_bind_int (res, 1, dbID);

            int step = sqlite3_step (res);

            // FIXME: assign values

            sqlite3_finalize (res);

            return 0;
        }

        else {
            logMsg (stderr, ERROR, NO_TYPE, 
                createString ("Failed to get weapon data for item: %i", dbID));
            #ifdef DEV
            logMsg (stderr, ERROR, NO_TYPE, createString ("DB error: %s", sqlite3_errmsg (itemsDb)));
            #endif
        } 
    }

    return weapon;

}

static void weapon_destroy (Weapon *weapon) { if (weapon) free (weapon); }

static Armour *armour_new (u32 dbID) {

    Armour *armour = (Armour *) malloc (sizeof (Armour));
    if (armour) {
        // get armour values from db
        sqlite3_stmt *res;
        char *sql = "SELECT * FROM Armour WHERE Id = ?";

        if (sqlite3_prepare_v2 (itemsDb, sql, -1, &res, 0) == SQLITE_OK) {
            sqlite3_bind_int (res, 1, dbID);

            int step = sqlite3_step (res);

            // FIXME: assign values

            sqlite3_finalize (res);

            return 0;
        }

        else {
            logMsg (stderr, ERROR, NO_TYPE, 
                createString ("Failed to get armour data for item: %i", dbID));
            #ifdef DEV
            logMsg (stderr, ERROR, NO_TYPE, createString ("DB error: %s", sqlite3_errmsg (itemsDb)));
            #endif
        } 
    }

    return armour;

}

static void armour_destroy (Armour *armor) { if (armor) free (armor); }

void *item_get_item_component (Item *item, ItemComponent component) {

    if (item) return item->components[component];

}

void item_remove_item_component (Item *item, ItemComponent component) {

    if (item) {
        switch (component) {
            case WEAPON_COMP: weapon_destroy (item->components[component]); break;
            case ARMOUR_COMP: armour_destroy (item->components[component]); break;

            default: break;
        }
    }

}

#pragma endregion

#pragma region ITEM

static void item_add_transform (GameObject *item_go) {

    if (item_go) game_object_add_component (item_go, TRANSFORM_COMP);

}

static u8 item_add_graphics (GameObject *item_go, Item *item) {

    if (item_go) {
        Graphics *graphics = game_object_add_component (item_go, GRAPHICS_COMP);
        if (graphics) {
            // reference the correct sprite sheet based on item type
            switch (item->type) {
                case ITEM_ACCESORY: graphics_ref_sprite_sheet (graphics, accesories); break;
                case ITEM_ARMOR: graphics_ref_sprite_sheet (graphics, armor); break;
                case ITEM_FOOD: graphics_ref_sprite_sheet (graphics, food); break;
                case ITEM_MISC: graphics_ref_sprite_sheet (graphics, misc); break;
                case ITEM_WEAPON: graphics_ref_sprite_sheet (graphics, weapons); break;

                default: break;
            }

            // get graphics values from db
            sqlite3_stmt *res;
            char *sql = "SELECT * FROM Graphics WHERE Id = ?";

            if (sqlite3_prepare_v2 (itemsDb, sql, -1, &res, 0) == SQLITE_OK) {
                sqlite3_bind_int (res, 1, item->dbID);

                int step = sqlite3_step (res);

                // FIXME: assign values

                sqlite3_finalize (res);

                return 0;
            }

            else {
                logMsg (stderr, ERROR, NO_TYPE, 
                    createString ("Failed to get graphics data for item: %i", item->dbID));
                #ifdef DEV
                logMsg (stderr, ERROR, NO_TYPE, createString ("DB error: %s", sqlite3_errmsg (itemsDb)));
                #endif
            } 
        }
    }

    return 1;

}

static void *item_add_component (Item *item, ItemComponent component) {

    void *retval = NULL;

    if (item) {
        switch (component) {
            case WEAPON_COMP: 
                retval = item->components[component] = weapon_new (item->dbID); 
                break;
            case ARMOUR_COMP: 
                retval = item->components[component] = armour_new (item->dbID);
                break;

            default: break;
        }
    }

    return retval;

}

GameObject *item_create (u32 dbID) {

    GameObject *item_go = game_object_new (NULL, "item");
    if (item_go) {
        Item *item = item_add_item_comp (item_go, dbID);

        item_add_transform (item_go);
        item_add_graphics (item_go, item);
    }

    return item_go;

}

#pragma endregion

// // search for the item in the inventory and remove it
// Item *removeFromInventory (Item *item) {

//     Item *retVal = NULL;
//     bool removed = false;

//     for (u8 y = 0; y < 3; y++) {
//         for (u8 x = 0; x < 7; x++) {
//             if (!removed) {
//                 if (item == main_player->inventory[x][y]) {
//                     retVal = main_player->inventory[x][y];
//                     main_player->inventory[x][y] = NULL;
//                     removed = true;
//                 }
//             }
            
//         }
//     }

//     return retVal;

// }

// /*** CREATING ITEMS ***/

// void healPlayer (void *);
// void toggleEquipWeapon (void *);
// void toggleEquipArmour (void *);

// // FIXME:
// EventListener getItemCallback (u8 cb) {

//     EventListener callback = NULL;

//     switch (cb) {
//         case 0: callback = NULL; break;
//         case 1: callback = healPlayer; break;
//         case 2: callback = toggleEquipWeapon; break;
//         case 3: callback = toggleEquipArmour; break;
//         default: break;
//     }

//     return callback;

// }

// // 29/08/2018 -- 23:34 -- new way of creating an item using sqlite db
// // 05/09/2018 -- 11:04 -- creating items with a more complex db
// Item *createItem (int itemId) {

//     // get the db data
//     sqlite3_stmt *res;
//     char *sql = "SELECT * FROM Items WHERE Id = ?";

//     if (sqlite3_prepare_v2 (itemsDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, itemId);
//     else {
//         fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (itemsDb));
//         return NULL;
//     } 

//     int step = sqlite3_step (res);

//     Item *item = newItem ();
//     if (item == NULL) return NULL;

//     item->dbId = itemId;

//     char name[50];
//     const char *itemName = sqlite3_column_text (res, ITEM_NAME_COL);
//     strcpy (name, itemName);
//     item->rarity = (u8) sqlite3_column_int (res, ITEM_RARITRY_COL);
//     item->value[0] = sqlite3_column_int (res, ITEM_GOLD_COL);
//     item->value[1] = sqlite3_column_int (res, ITEM_SILVER_COL);
//     item->value[2] = sqlite3_column_int (res, ITEM_COPPER_COL);
//     item->probability = sqlite3_column_double (res, ITEM_PROB_COL);
//     item->stackable = (sqlite3_column_int (res, ITEM_STACKABLE_COL) == 0) ? false : true;
//     item->quantity = (u8) sqlite3_column_int (res, ITEM_QUANTITY_COL);

//     // FIXME: 
//     item->callback = getItemCallback ((u8) sqlite3_column_int (res, ITEM_CALLBACK_COL));

//     // graphics
//     if (addGraphicsToItem (itemId, item, name) != 0) {
//         fprintf (stderr, "Error adding graphics component!\n");
//         return NULL;
//     } 

//     sqlite3_finalize (res);

//     return item;

// } 

// Item *createWeapon (u32 itemId) {

//     Item *weapon = createItem (itemId);

//     weapon->stackable = false;
//     weapon->quantity = 1;

//     // get the db data
//     sqlite3_stmt *res;
//     char *sql = "SELECT * FROM Weapons WHERE Id = ?";

//     if (sqlite3_prepare_v2 (itemsDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, itemId);
//     else {
//         fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (itemsDb));
//         return NULL;
//     } 

//     int step = sqlite3_step (res);

//     Weapon w;
//     w.dbId = itemId;
//     w.type = (u8) sqlite3_column_int (res, 1);
//     w.twoHanded = (sqlite3_column_int (res, 2) == 0) ? false : true;
//     w.dps = (u8) sqlite3_column_int (res, 3);
//     w.slot = sqlite3_column_int (res, 4);
//     w.maxLifetime = (u16) sqlite3_column_int (res, 5);
//     w.lifetime = w.maxLifetime;
//     w.isEquipped = false;
//     addItemComp (weapon, WEAPON, &w);

//     sqlite3_finalize (res);

//     return weapon;

// }

// Item *createArmour (u32 itemId) {

//     Item *armour = createItem (itemId);

//     armour->stackable = false;
//     armour->quantity = 1;

//     // get the db data
//     sqlite3_stmt *res;
//     char *sql = "SELECT * FROM Armour WHERE Id = ?";

//     if (sqlite3_prepare_v2 (itemsDb, sql, -1, &res, 0) == SQLITE_OK) sqlite3_bind_int (res, 1, itemId);
//     else {
//         fprintf (stderr, "Error! Failed to execute statement: %s\n", sqlite3_errmsg (itemsDb));
//         return NULL;
//     } 

//     int step = sqlite3_step (res);

//     Armour a;
//     a.dbId = itemId;
//     a.type = (u8) sqlite3_column_int (res, 1);
//     a.slot = (u8) sqlite3_column_int (res, 2);
//     a.maxLifetime = (u16) sqlite3_column_int (res, 3);
//     a.lifetime = a.maxLifetime;
//     a.isEquipped = false;
//     addItemComp (armour, WEAPON, &a);

//     sqlite3_finalize (res);

//     return armour;

// }

// /*** ITEMS FUNCS ***/

// DoubleList *getItemsAtPos (u8 x, u8 y) {

//     Position *pos = NULL;
//     if (items == NULL || (LIST_SIZE (items) == 0)) return NULL;

//     DoubleList *retVal = dlist_init (free);
//     for (ListElement *e = LIST_START (items); e != NULL; e = e->next) {
//         pos = (Position *) getGameComponent ((Item *) e->data, POSITION);
//         // inventory items do NOT have a pos comp
//         if (pos != NULL) {
//             if (pos->x == x && pos->y == y) dlist_insert_after (retVal, NULL, e->data);
//         }
//     }

//     return retVal;

// }

// u32 getItemColor (u8 rarity) {

//     u32 color;
//     switch (rarity) {
//         case 0: color = RUBISH_COLOR; break;
//         case 1: color = COMMON_COLOR; break;
//         case 2: color = RARE_COLOR; break;
//         case 3: color = EPIC_COLOR; break;
//         case 4: color = LEGENDARY_COLOR; break;
//         default: color = COMMON_COLOR; break;
//     }

//     return color;

// }

// char *getItemSlot (Item *item) {

//     char slot[15];

//     Weapon *w = getItemComponent (item, WEAPON);
//     if (w != NULL) {
//         switch (w->slot) {
//             case 0: strcpy (slot, "Main"); break;
//             case 1: strcpy (slot, "Off"); break;
//             default: break;
//         }
//     }

//     else {
//         Armour *a = getItemComponent (item, ARMOUR);
//         if (a != NULL) {
//             switch (a->slot) {
//                 case 0: strcpy (slot, "Head"); break;
//                 case 1: strcpy (slot, "Necklace"); break;
//                 case 2: strcpy (slot, "Shoulders"); break;
//                 case 3: strcpy (slot, "Cape"); break;
//                 case 4: strcpy (slot, "Chest"); break;
//                 case 5: strcpy (slot, "Hands"); break;
//                 case 6: strcpy (slot, "Waist"); break;
//                 case 7: strcpy (slot, "Legs"); break;
//                 case 8: strcpy (slot, "Shoes"); break;
//                 case 9: strcpy (slot, "Ring"); break;
//                 default: break;
//             }
//         }
//     }

//     char *retVal = (char *) calloc (strlen (slot), sizeof (char));
//     strcpy (retVal, slot);

//     return retVal;

// }

// // FIXME:
// char *getEquipmentTypeName (Item *item) {

//     char slot[15];

//     Weapon *w = getItemComponent (item, WEAPON);
//     if (w != NULL) {
//         switch (w->type) {
//             case 0: strcpy (slot, "Sword"); break;
//             // case 1: strcpy (slot, "Off"); break;
//             default: break;
//         }
//     }

//     char *retVal = (char *) calloc (strlen (slot), sizeof (char));
//     strcpy (retVal, slot);

//     return retVal;

// }

// bool itemStacked (Item *item) {

//     bool stacked = false;

//     for (u8 y = 0; y < 3; y++) {
//         for (u8 x = 0; x < 7; x++) {
//             if (main_player->inventory[x][y] != NULL && !stacked) {
//                 if (main_player->inventory[x][y]->dbId == item->dbId) {
//                     if (main_player->inventory[x][y]->quantity < MAX_STACK) {
//                         main_player->inventory[x][y]->quantity += 1;
//                         destroyItem (item);
//                         stacked = true;
//                     }
//                 }
//             }
//         }
//     }

//     return stacked;

// }

// void addToInventory (Item *item) {

//     bool inserted = false;

//     if (item->stackable && inventoryItems > 0) {
//         // insert in the next available inventory slot
//         if (!itemStacked (item)) {
//             for (u8 y = 0; y < 3; y++) {
//                 for (u8 x = 0; x < 7; x++) {
//                     if (main_player->inventory[x][y] == NULL && !inserted) {
//                         main_player->inventory[x][y] = item;
//                         inserted = true;
//                         inventoryItems += 1;
//                     }
//                 }
//             }
//         }
//     }

//     // insert in the next available inventory slot
//     else {
//         for (u8 y = 0; y < 3; y++) {
//             for (u8 x = 0; x < 7; x++) {
//                 if (main_player->inventory[x][y] == NULL && !inserted) {
//                     main_player->inventory[x][y] = item;
//                     inserted = true;
//                     inventoryItems += 1;
//                 }
//             }
//         }
//     } 

// }

// // pickup the first item of the list
// void pickUp (Item *item) {

//     if (item != NULL) {
//         addToInventory (item);
            
//         // remove the item from the map
//         removeGameComponent (item, POSITION);

//         Graphics *g = (Graphics *) getGameComponent (item, GRAPHICS);
//         if (g != NULL) {
//             if (g->name != NULL) logMessage (createString ("You picked up the %s.", g->name), DEFAULT_COLOR);
//             else logMessage ("Picked up the item!", DEFAULT_COLOR);
//         }

//         playerTookTurn = true;
//     }

// }

// // As of 16/08/2018:
// // The character must be on the same coord as the item to be able to pick it up
// void getItem (void) {

//     // get a list of items nearby the player
//     DoubleList *objects = getItemsAtPos (main_player->pos->x, main_player->pos->y);

//     if (objects == NULL || (LIST_SIZE (objects) <= 0)) {
//         if (objects != NULL) free (objects);
//         logMessage ("There are no items here!", WARNING_COLOR);
        
//     }

//     // we only pick one item each time
//     else {
//         pickUp ((Item *) dlist_remove_element (objects, (LIST_START (objects))));
//         if (objects) dlist_clean (objects);
//     } 

// }

// extern Loot *currentLoot;

// void getLootItem (u8 lootYIdx) {

//     if (currentLoot != NULL) {
//         // we only pick one item each time
//         if ((currentLoot->lootItems != NULL) && (LIST_SIZE (currentLoot->lootItems) > 0)) {
//             Item *item = NULL;
//             u8 count = 0;
//             for (ListElement *e = LIST_START (currentLoot->lootItems); e != NULL; e = e->next) {
//                 if (count == lootYIdx) {
//                     item = (Item *) dlist_remove_element (currentLoot->lootItems, e);
//                     break;
//                 }

//                 count++;
//             }

//             pickUp (item);

//             // update Loot UI
//             updateLootUI (lootYIdx);

//             // FIXME:
//             // update tooltip UI
//             // if (tooltipView != NULL) if (lootItem == item) toggleTooltip (true);
            
//         }

//         else logMessage ("There are no items to pick up!", WARNING_COLOR);
//     }

//     else logMessage ("There is not loot available here!", WARNING_COLOR);

// }

// // TODO: add the option to drop the entiere stack
// void dropItem (Item *item) {

//     if (item == NULL) return;
//     if (item->quantity <= 0) return;    // quick dirty fix

//     Item *dropItem = NULL;

//     if (item->stackable) {
//         if (item->quantity > 1) {
//             item->quantity--;
//             dropItem = createItem (item->dbId);
//         } 

//         else dropItem = removeFromInventory (item);
//     }

//     else dropItem = removeFromInventory (item);

//     inventoryItems--;

//     // update the UI
//     resetInventoryRects ();

//     Position pos = { .x = main_player->pos->x, .y = main_player->pos->y, .layer = MID_LAYER };
//     addGameComponent (dropItem, POSITION, &pos);

//     Graphics *g = (Graphics *) getGameComponent (dropItem, GRAPHICS);
//     if (g != NULL) {
//         char *msg = createString ("You dropped the %s.", g->name);
//         logMessage (msg, DEFAULT_COLOR);
//         free (msg);
//     }

//     else logMessage ("You dropped the item.", DEFAULT_COLOR);

// }

// /*** WEAPONS -- ARMOUR ***/

// // FIXME: add more types
// char *getItemTypeName (Item *item) {

//     char typeName[15];

//     Weapon *weapon = (Weapon *) getItemComponent (item, WEAPON);
//     if (weapon != NULL) {
//         switch (weapon->type) {
//             case 0: strcpy (typeName, "Sword"); break;
//             case 1: break;
//             case 2: break;
//             default: break;
//         }
//     }

//     char *retVal = (char *) calloc (strlen (typeName), sizeof (char));
//     strcpy (retVal, typeName);

//     return retVal;

// }

// // TODO: check for specific class weapons
// // TODO: update combat stats based on weapon modifiers if necessary
// void toggleEquipWeapon (void *i) {

//     if (i == NULL) return;

//     Item *item = (Item *) i;
//     Weapon *weapon = (Weapon *) getItemComponent (item, WEAPON);
//     if (weapon == NULL) {
//         fprintf (stderr, "No weapon component!!\n");
//         return;
//     }

//     // unequip
//     if (weapon->isEquipped) {
//         Item *w = main_player->weapons[weapon->slot];
//         if (w != NULL) {
//             addToInventory (w);

//             Graphics *g = (Graphics *) getGameComponent (w, GRAPHICS);
//             if (g != NULL) {
//                 char *str = createString ("You unequip the %s", g->name);
//                 logMessage (str, DEFAULT_COLOR);
//                 free (str);
//             } 

//             main_player->weapons[weapon->slot] = NULL;

//             weapon->isEquipped = false;
//         }
//     }

//     // equip
//     else {
//         Item *w = removeFromInventory (item);
//         if (w != NULL) {
//             // unequip our current weapon if we have one
//             if (main_player->weapons[weapon->slot] != NULL) 
//                 toggleEquipWeapon (main_player->weapons[weapon->slot]);

//             // if we are equipping a two handed and we have two one handed
//             if (((Weapon *) getItemComponent (w, WEAPON))->twoHanded) {
//                 if (main_player->weapons[1] != NULL)
//                     toggleEquipWeapon (main_player->weapons[1]); // unequip the off hand weapon

//             }

//             main_player->weapons[weapon->slot] = w;
//             Graphics *g = (Graphics *) getGameComponent (w, GRAPHICS);
//             if (g != NULL) {
//                 char *str = createString ("You are now wielding the %s.", g->name);
//                 logMessage (str, DEFAULT_COLOR);
//                 free (str);
//             }

//             else logMessage ("You equip the weapon.", DEFAULT_COLOR);

//             weapon->isEquipped = true;
//         }
//     }

//     // update the UI
//     resetCharacterRects ();
//     resetInventoryRects ();
        
// }

// // TODO: update combat stats based on armour modifiers if necessary
// // TODO: create better strings deppending on the item
// void toggleEquipArmour (void *i) {

//     if (i == NULL) return;

//     Item *item = (Item *) i;
//     Armour *armour = (Armour *) getItemComponent (item, ARMOUR);
//     if (armour == NULL) {
//         fprintf (stderr, "No armour component!!\n");
//         return;
//     }

//     // unequip
//     if (armour->isEquipped) {
//         Item *a = main_player->equipment[armour->slot];
//         if (a != NULL) {
//             addToInventory (a);
//             Graphics *g = (Graphics *) getGameComponent (a, GRAPHICS);
//             if (g != NULL) {
//                 char *str = createString ("You take off the %s.", g->name);
//                 logMessage (str, DEFAULT_COLOR);
//                 free (str);
//             } 

//             main_player->equipment[armour->slot] = NULL;

//             armour->isEquipped = false;
//         }
//     }

//     // equip
//     else {
//         Item *a = removeFromInventory (item);
//         if (a != NULL) {
//             // unequip the armour in that slot if we have one
//             if (main_player->equipment[armour->slot] != NULL)
//                 toggleEquipArmour (main_player->equipment[armour->slot]);

//             main_player->equipment[armour->slot] = a;
//             Graphics *g = (Graphics *) getGameComponent (a, GRAPHICS);
//             if (g != NULL) {
//                 char *str = createString ("You are now wielding the %s.", g->name);
//                 logMessage (str, DEFAULT_COLOR);
//                 free (str);
//             }

//             armour->isEquipped = true;
//         }
//     }

//     // update the UI
//     resetCharacterRects ();
//     resetInventoryRects ();

// }

// // TODO: crafting

// // TODO: I think we will want to add the ability to repair your items in a shop,
// // but only if they are above 0, if you don't repair your items soon enough, 
// // you will lose them
// void repairItems (void) {

// }

// // 18/09/2018 -- i don't like this function
// u32 getLifeTimeColor (Item *item) {

//     u32 color = WHITE;

//     Weapon *weapon = (Weapon *) getItemComponent (item, WEAPON);
//     if (weapon != NULL) {
//         if (weapon->lifetime >= (weapon->maxLifetime * 0.75)) color = FULL_GREEN;
//         else if ((weapon->lifetime < (weapon->maxLifetime * 0.75)) && (weapon->lifetime >= (weapon->maxLifetime * 0.25)))
//             color = YELLOW;
//         else color = FULL_RED;
//     }

//     else {
//         Armour *armour = (Armour *) getItemComponent (item, ARMOUR);
//         if (armour != NULL) {
//             if (armour->lifetime >= (armour->maxLifetime * 0.75)) color = FULL_GREEN;
//             else if ((armour->lifetime < (armour->maxLifetime * 0.75)) && (armour->lifetime >= (armour->maxLifetime * 0.25)))
//                 color = YELLOW;
//             else color = FULL_RED;
//         }
//     }

//     return color;

// }

// // As of 30/08/2018 -- 11:10 -- if an item get to lifetime = 0, it will unequip and 
// // go to the inventory... if there is no space in the inventory, it will stay equipped
// // but it will loose all of its qualities

// // FIXME:
// // only reduce lifetime of weapons, and equipment
// void updateLifeTime (void) {

//     Item *item = NULL;

//     // weapons 
//     Weapon *weapon = NULL;
//     for (u8 i = 0; i < 3; i++) {
//         item = main_player->weapons[i];
//         weapon = (Weapon *) getItemComponent (item, WEAPON);

//         // FIXME: update lifetime of weapons when hitting a mob

//         if (weapon->lifetime <= 0) {
//             // unequip
//             // toggleEquipWeapon (item);
//         }
//     }

//     // TODO:
//     // armour
//     // Armour *armour = NULL;
//     // for (u8 i = 0; i < 9; i++) {
//     //     item = playerComp->equipment[i];
//     //     armour = (Armour *) getItemComponent (item, ARMOUR);

//     //     // FIXME: update lifetime of armour when being hit from a mob

//     //     if (armour->lifetime <= 0) {
            
//     //     }
//     // }

// }

// /*** CALLBACKS ***/

// void healPlayer (void *i) {

//     Item *item = (Item *) i;

//     i32 *currHealth = &main_player->combat->baseStats.health;
//     u32 maxHealth = main_player->combat->baseStats.maxHealth;

//     // FIXME: get the real data
//     // FIXME: 09/09/2018 -- 23:08 -- this is just for testing
//     u16 health = (u16) randomInt (5, 10);

//     if (*currHealth == maxHealth) logMessage ("You already have full health.", WARNING_COLOR);
//     else {
//         u16 realHp;

//         *currHealth += health;

//         // clamp the value if necessary
//         if (*currHealth > maxHealth) {
//             realHp = health - (*currHealth - maxHealth);
//             *currHealth = maxHealth;
//         }

//         else realHp = health;  

//         item->quantity--;
//         if (item->quantity == 0) {
//             Item *old = removeFromInventory (item);
//             if (old != NULL) destroyItem (old);

//             resetInventoryRects ();
//         }

//         // TODO: maybe better strings
//         // you have ate the apple for 5 health
//         char *str = createString ("You have been healed by %i hp.", realHp);
//         logMessage (str, SUCCESS_COLOR);
//         free (str);
//     }

// }

// /*** CLEAN UP ***/

// // FIXME: problems with items pool
// void cleanUpItems (void) {

//     dlist_destroy (items);
//     // clearPool (itemsPool);

//     // disconnect from the db
//     sqlite3_close (itemsDb);

// }

// /*** DEVELOPMENT - DATABASE ***/

// // 05/09/2018 -- new way for creating our items db 
// void createItemsDb (void) {

//     char *err_msg = 0;

//     // master item
//     char *sql = "DROP TABLE IF EXISTS Items;"
//                 "CREATE TABLE Items(Id INT, Name TEXT, Rarity INT, Gold INT, Silver INT, Copper INT, Probability DOUBLE, Quantity INT, Stackable INT, Callback INT);";

//     if (sqlite3_exec (itemsDb, sql, 0, 0, &err_msg) != SQLITE_OK) {
//         fprintf (stderr, "Error! Failed create ITEMS table!\n");
//         fprintf (stderr, "SQL error: %s\n", err_msg);
//         sqlite3_free (err_msg);
//     }

//     // graphics
//     sql = "DROP TABLE IF EXISTS Graphics;"
//           "CREATE TABLE Graphics(Id INT, Glyph INT, Color TEXT)";

//     if (sqlite3_exec (itemsDb, sql, 0, 0, &err_msg) != SQLITE_OK) {
//         fprintf (stderr, "Error! Failed create GRAPHICS table!\n");
//         fprintf (stderr, "SQL error: %s\n", err_msg);
//         sqlite3_free (err_msg);
//     }

//     // weapons
//     sql = "DROP TABLE IF EXISTS Weapons;"
//           "CREATE TABLE Weapons(Id INT, Type INT, Two INT, Dps INT, Slot INT, Lifetime INT);";

//     if (sqlite3_exec (itemsDb, sql, 0, 0, &err_msg) != SQLITE_OK) {
//         fprintf (stderr, "Error! Failed create WEAPONS table!\n");
//         fprintf (stderr, "SQL error: %s\n", err_msg);
//         sqlite3_free (err_msg);
//     }

//     // armour
//     sql = "DROP TABLE IF EXISTS Armour;"
//           "CREATE TABLE Armour(Id INT, Type INT, Slot INT, Lifetime INT);";

//     if (sqlite3_exec (itemsDb, sql, 0, 0, &err_msg) != SQLITE_OK) {
//         fprintf (stderr, "Error! Failed create ARMOUR table!\n");
//         fprintf (stderr, "SQL error: %s\n", err_msg);
//         sqlite3_free (err_msg);
//     }

// }
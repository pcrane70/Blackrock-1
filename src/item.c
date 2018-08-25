#include "blackrock.h"
#include "game.h"

#include "utils/list.h"
#include "objectPool.h"

#include "item.h"

#include "config.h"

#include "ui/gameUI.h" // for strings

#include "utils/myUtils.h"

List *items = NULL;
Pool *itemsPool = NULL;

extern Config *itemsConfig;

static u32 itemsId = 0;

void initItems (void) {

    items = initList (free);
    itemsPool = initPool ();

}

Item *newItem (void) {

    Item *i = NULL;

    // check if there is a an available one in the items pool
    if (POOL_SIZE (itemsPool) > 0) {
        i = pop (itemsPool);
        if (i == NULL) i = (Item *) malloc (sizeof (Item));
    } 
    else i = (Item *) malloc (sizeof (Item));

    if (i != NULL) {
        i->id = itemsId;
        itemsId++;
        for (u8 u = 0; u < 2; u++) i->components[u] = NULL;
        insertAfter (items, LIST_END (items), i);
    }

    return i;

}

void *getItemComp (Item *item, GameComponent type) {

    void *retVal = item->components[type];
    if (retVal == NULL) return NULL;
    else return retVal;

}

void addItemComp (Item *item, GameComponent type, void *data) {

    if (item == NULL || data == NULL) return;

    switch (type) {
        case POSITION: {
            if (getItemComp (item, type) != NULL) return;
            Position *newPos = NULL;
            if (POOL_SIZE (posPool) > 0) {
                newPos = pop (posPool);
                if (newPos == NULL) newPos = (Position *) malloc (sizeof (Position));
            }
            else newPos = (Position *) malloc (sizeof (Position));

            Position *posData = (Position *) data;
            newPos->objectId = item->id;
            newPos->x = posData->x;
            newPos->y = posData->y;
            newPos->layer = posData->layer;

            item->components[type] = newPos;
            insertAfter (positions, NULL, newPos);
        } break;
        case GRAPHICS: {
            if (getItemComp (item, type) != NULL) return;
            Graphics *newGraphics = NULL;
            if (POOL_SIZE (graphicsPool) > 0) {
                newGraphics = pop (graphicsPool);
                if (newGraphics == NULL) newGraphics = (Graphics *) malloc (sizeof (Graphics));
            }
            else newGraphics = (Graphics *) malloc (sizeof (Graphics));

            Graphics *graphicsData = (Graphics *) data;
            newGraphics->objectId = item->id;
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

void removeItemComp (Item *item, GameComponent type) {

    if (item == NULL) return;

    switch (type) {
        case POSITION: {
            Position *posComp = (Position *) getItemComp (item, type);
            if (posComp == NULL) return;
            ListElement *e = getListElement (positions, posComp);
            void *posData = NULL;
            if (e != NULL) posData = removeElement (positions, e);
            push (posPool, posData);
            item->components[type] = NULL;
        } break;
        case GRAPHICS: {
            Graphics *graComp = (Graphics *) getItemComp (item, type);
            if (graComp == NULL) return;
            ListElement *e = getListElement (graphics, graComp);
            void *graData = NULL;
            if (e != NULL) graData = removeElement (graphics, e);
            push (graphicsPool, graData);
            item->components[type] = NULL;
        } break;
        default: break;
    }

}

Item *createItem (u16 itemId) {

    ConfigEntity *itemEntity = getEntityWithId (itemsConfig, itemId);
    if (itemEntity == NULL) return NULL;

    Item *item = newItem ();

    asciiChar glyph = atoi (getEntityValue (itemEntity, "glyph"));
    char *name = getEntityValue (itemEntity, "name");
    u32 color = xtoi (getEntityValue (itemEntity, "color"));
    Graphics g = { 0, glyph, color, 0x000000FF, false, false, name };
    addItemComp (item, GRAPHICS, &g);

    item->type = atoi (getEntityValue (itemEntity, "type"));
    item->rarity = atoi (getEntityValue (itemEntity, "rarity"));
    item->quantity = atoi (getEntityValue (itemEntity, "quantity"));
    item->weight = atoi (getEntityValue (itemEntity, "weight"));
    item->value[0] = atoi (getEntityValue (itemEntity, "gold"));
    item->value[1] = atoi (getEntityValue (itemEntity, "silver"));
    item->value[2] = atoi (getEntityValue (itemEntity, "copper"));
        
    return item;

}

Weapon *createWeapon (u16 itemId) {

    ConfigEntity *itemEntity = getEntityWithId (itemsConfig, itemId);
    if (itemEntity == NULL) return NULL;

    Weapon *weapon = (Weapon *) malloc (sizeof (Weapon));

    weapon->item = createItem (itemId);
    if (weapon->item == NULL) return NULL;

    weapon->dps = atoi (getEntityValue (itemEntity, "dps"));
    weapon->maxLifetime = atoi (getEntityValue (itemEntity, "maxLifetime"));
    weapon->lifetime = weapon->maxLifetime;
    weapon->isEquipped = false;

    return weapon;

}

void destroyItem (Item *item) {

    for (u8 i = 0; i < ITEM_COMPS; i++) removeItemComp (item, i);

    ListElement *e = getListElement (items, item);
    if (e != NULL) {
        void *data = removeElement (items, e);
        push (itemsPool, data);
    }

}

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
        pos = (Position *) getItemComp ((Item *) e->data, POSITION);
        // inventory items do NOT have a pos comp
        if (pos != NULL) {
            if (pos->x == x && pos->y == y) insertAfter (retVal, NULL, e->data);
        }
    }

    return retVal;

}

// pickup the first item of the list
void pickUp (List *lootItems, u8 yIdx) {

    Item *item = NULL;
    u8 count = 0;
    for (ListElement *e = LIST_START (lootItems); e != NULL; e = e->next) {
        if (count == yIdx) {
            item = (Item *) removeElement (lootItems, LIST_START (lootItems));
            break;
        }

        count++;
    }

    if (item != NULL) {
        if ((getCarriedWeight () + item->weight) <= playerComp->maxWeight) {
            // add the item to the inventory
            insertAfter (playerComp->inventory, NULL, item);
            // remove the item from the map
            removeItemComp (item, POSITION);

            Graphics *g = (Graphics *) getItemComp (item, GRAPHICS);
            if (g != NULL) {
                if (g->name != NULL) logMessage (createString ("You picked up the %s.", g->name), SUCCESS_COLOR);
                else logMessage ("Picked up the item!", SUCCESS_COLOR);
            }

            playerTookTurn = true;

            // update Loot UI
            updateLootUI (yIdx);
        }

        else logMessage ("You are carrying to much already!", WARNING_COLOR);
    }

}


// As of 16/08/2018:
// The character must be on the same coord as the item to be able to pick it up
// FIXME:
void getItem (void) {

    Position *playerPos = (Position *) getComponent (player, POSITION);
    // get a list of items nearby the player
    // List *objects = getItemsAtPos (playerPos->x, playerPos->y);
    List *objects = NULL;

    if (objects == NULL || (LIST_SIZE (objects) <= 0)) {
        if (objects != NULL) destroyList (objects);
        logMessage ("There are no items here!", WARNING_COLOR);
        return;
    }

    // we only pick one item each time
    // pickUp (objects, 0);

    if (objects != NULL) destroyList (objects);

}

extern Loot *currentLoot;

void getLootItem (u8 lootYIdx) {

    if (currentLoot != NULL) {
        // we only pick one item each time
        if ((currentLoot->lootItems != NULL) && (LIST_SIZE (currentLoot->lootItems) > 0)) 
            pickUp (currentLoot->lootItems, lootYIdx);

        else logMessage ("There are no items to pick up!", WARNING_COLOR);
    }

    else logMessage ("There is not loot available here!", WARNING_COLOR);

}

// FIXME:
// TODO: how do we select which item to drop?
void dropItem (Item *item) {

    if (item == NULL) return;

    Position *playerPos = (Position *) getComponent (player, POSITION);

    Position pos = { .x = playerPos->x, .y = playerPos->y, .layer = MID_LAYER };
    addItemComp (item, POSITION, &pos);

    // FIXME: unequip item

    // remove from the inventory
    // FIXME:
    // ListElement *e = getListElement (playerComp->inventory, go);
    // if (e != NULL) removeElement (playerComp->inventory, e);

    Graphics *g = (Graphics *) getItemComp (item, GRAPHICS);
    if (g != NULL) {
        char *msg = createString ("You dropped the %s.", g->name);
        logMessage (msg, SUCCESS_COLOR);
        free (msg);
    }
    else logMessage ("You dropped the item.", SUCCESS_COLOR);

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

// TODO: consumables

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
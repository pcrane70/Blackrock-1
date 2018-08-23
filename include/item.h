#ifndef ITEM_H
#define ITEM_H

#include "utils/list.h"
#include "objectPool.h"

extern List *items;
extern Pool *itemsPool;

// TODO: how can we handle consumables?
// TODO: how can we handle crafting?
typedef struct Item {

    u16 id;
    u16 itemId;     // item's unique identifire in our db
    u8 type;        // consumable, weapon, etc?
    u8 rarity;      // epic, rare, common, rubish, etc.
    u8 quantity;    // this is used to handle stacks, max stack is 20
    u8 weight;      // we have a max weight that we can carry based on our class, genre, etc
    u16 value[3];   // gold, silver, copper
    // FIXME: effects
    // we only need position and graphics
    void *components[2];   // 23/08/2018 -- 7:12 -- testing a separate ECS for our items

} Item;

// 23/08/2018 -- 6:55 -- testing how does this works
typedef struct Armour {

    Item *item;          // inherites from item
    u16 maxLifetime;
    u16 lifetime;
    char *slot;
    bool isEquipped;

} Armour;

typedef struct Weapon {

    Item *item;          // inherites from item  
    u8 dps;
    u16 maxLifetime;
    u16 lifetime;
    bool isEquipped;
    // FIXME: how do we handle if it is one or two handed??

} Weapon;

extern void getItem (void);

#endif
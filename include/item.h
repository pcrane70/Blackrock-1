#ifndef ITEM_H
#define ITEM_H

#include "utils/list.h"
#include "objectPool.h"

/*** ITEM RARITY ***/

#define RUBISH_COLOR        0x798679FF
#define COMMON_COLOR        0x0CB21DFF
#define RARE_COLOR          0x2338B2FF
#define EPIC_COLOR          0x6B3CADFF
#define LEGENDARY_COLOR     0xD65300FF

typedef enum ItemComponent {

    WEAPON = 0,
    ARMOUR

} ItemComponent;

#define ITEM_COMPS  2


#define GAME_OBJECT_COMPS   2

extern List *items;
extern Pool *itemsPool;

typedef struct Item {

    u16 itemId;     
    u16 dbId;       // item's unique identifire in our db
    u8 type;        // consumable, weapon, etc?
    u8 rarity;      // epic, rare, common, rubish, etc.
    u8 quantity;    // this is used to handle stacks, max stack is 20
    u8 weight;      // we have a max weight that we can carry based on our class, genre, etc
    u16 value[3];   // gold, silver, copper
    // FIXME: effects   
    void *components[GAME_OBJECT_COMPS];  
    void *itemComps[ITEM_COMPS];

} Item;

// 23/08/2018 -- 6:55 -- testing how does this works
typedef struct Armour {

    u16 itemId;     
    u16 dbId;       // item's unique identifire in our db
    u16 maxLifetime;
    u16 lifetime;
    char *slot;
    bool isEquipped;

} Armour;

typedef struct Weapon {

    u16 itemId;     
    u16 dbId;       // item's unique identifire in our db
    u8 dps;
    u16 maxLifetime;
    u16 lifetime;
    bool isEquipped;
    // FIXME: how do we handle if it is one or two handed??

} Weapon;

extern void initItems (void);

extern Item *createItem (u16 itemId);
extern void getItem (void);
extern void getLootItem (u8);
extern void *getItemComp (Item *, GameComponent);
extern u32 getItemColor (u8 rarity);
extern u16 getCarriedWeight (void);

extern void cleanUpItems (void);

#endif
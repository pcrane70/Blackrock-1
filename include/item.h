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

#define MAX_STACK   20

typedef struct Item {

    u16 itemId;     
    u16 dbId;       // item's unique identifire in our db
    u8 type;        // consumable, weapon, etc?
    u8 rarity;      // epic, rare, common, rubish, etc.
    bool stackable; // this is used to handle stacks, max stack is 20
    u8 quantity;    
    u16 value[3];   // gold, silver, copper
    EventListener callback;   
    void *components[GAME_OBJECT_COMPS];  
    void *itemComps[ITEM_COMPS];

} Item;

// 23/08/2018 -- 6:55 -- testing how does this works
typedef struct Armour {

    u16 itemId;     
    u16 dbId;       // item's unique identifire in our db
    u16 maxLifetime;
    u16 lifetime;
    u8 slot;
    bool isEquipped;

} Armour;

// TODO: add class specific weapons
typedef struct Weapon {

    u16 itemId;     
    u16 dbId;       // item's unique identifire in our db
    u8 dps;
    u16 maxLifetime;
    u16 lifetime;
    bool isEquipped;
    u8 slot;
    bool twoHanded;

} Weapon;

extern void initItems (void);

extern Item *createItem (u16 itemId);
extern void getItem (void);
extern void dropItem (Item *);
extern void getLootItem (u8);
extern void *getGameComponent (Item *, GameComponent);
extern void *getItemComponent (Item *, ItemComponent);
extern u32 getItemColor (u8 rarity);
extern u16 getCarriedWeight (void);

extern Item *deleteItem (Item *);

extern void cleanUpItems (void);

#endif
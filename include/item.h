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

/*** DB COLS ***/

#define ITEM_ID_COL          0
#define ITEM_NAME_COL        1
#define ITEM_RARITRY_COL     2
#define ITEM_GOLD_COL        3
#define ITEM_SILVER_COL      4
#define ITEM_COPPER_COL      5
#define ITEM_PROB_COL        6
#define ITEM_QUANTITY_COL    7  
#define ITEM_STACKABLE_COL   8
#define ITEM_CALLBACK_COL    9

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

    u32 itemId;     
    u32 dbId;       // item's unique identifire in our db
    u8 type;        // consumable, weapon, etc?
    u8 rarity;      // epic, rare, common, rubish, etc.
    bool stackable; // this is used to handle stacks, max stack is 20
    u8 quantity;    
    u32 value[3];   // gold, silver, copper
    double probability;
    EventListener callback;   
    void *components[GAME_OBJECT_COMPS];  
    void *itemComps[ITEM_COMPS];

} Item;

// 23/08/2018 -- 6:55 -- testing how does this works
typedef struct Armour {

    u32 itemId;     
    u32 dbId;       // item's unique identifire in our db
    u8 type;
    u32 maxLifetime;
    u32 lifetime;
    u8 slot;
    bool isEquipped;

} Armour;

// TODO: add class specific weapons
typedef struct Weapon {

    u32 itemId;     
    u32 dbId;       // item's unique identifire in our db
    u8 type;
    u8 dps;
    u32 maxLifetime;
    u32 lifetime;
    bool isEquipped;
    u8 slot;
    bool twoHanded;

} Weapon;

extern void initItems (void);

extern Item *createItem (int itemId);
extern Item *createWeapon (u32 itemId);

extern void getItem (void);
extern void dropItem (Item *);
extern void getLootItem (u8);
extern void *getGameComponent (Item *, GameComponent);
extern void *getItemComponent (Item *, ItemComponent);

extern u32 getItemColor (u8 rarity);
extern char *getItemSlot (Item *);

extern Item *destroyItem (Item *);
extern Item *deleteItem (Item *);

extern void cleanUpItems (void);

#endif
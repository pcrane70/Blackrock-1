#ifndef ITEM_H
#define ITEM_H

#include "collections/dlist.h"
#include "utils/objectPool.h"

#define ITEMS_PIXEL_SIZE        16

/*** ITEM RARITY COLOR ***/

#define RUBISH_COLOR        0x798679FF
#define COMMON_COLOR        0x0CB21DFF
#define RARE_COLOR          0x2338B2FF
#define EPIC_COLOR          0x6B3CADFF
#define LEGENDARY_COLOR     0xD65300FF

/*** DB COLS ***/

#define DB_COL_ITEM_ID              0
#define DB_COL_ITEM_NAME            1
#define DB_COL_ITEM_RARITY          2
#define DB_COL_ITEM_GOLD            3
#define DB_COL_ITEM_SILVER          4
#define DB_COL_ITEM_COPPER          5
#define DB_COL_ITEM_COL             6
#define DB_COL_ITEM_QUANTITY        7
#define DB_COL_ITEM_STACKABLE       8
#define DB_COL_ITEM_CALLBACK        9

#define MAX_STACK           20

extern u8 items_connect_db (void);
extern u8 items_init (void);

typedef enum ItemRarity {

    COMMON,

} ItemRarity;

typedef enum ItemType {

    ITEM_ACCESORY,
    ITEM_ARMOR,
    ITEM_FOOD,
    ITEM_MISC,
    ITEM_WEAPON

} ItemType;

typedef enum ItemComponent {

    WEAPON_COMP = 0,
    ARMOUR_COMP

} ItemComponent;

#define ITEM_COMPS  2

// component for a game object
typedef struct Item {

    u32 goID;
    u32 dbID;                       // item's unique identifire in our db
    
    ItemType type;                  // consumable, weapon, etc
    u8 rarity;                      // epic, rare, common, rubish, etc.
    bool stackable;                 // max stack is 20
    u8 quantity;                      
    u32 value[3];                   // gold, silver, copper
    double probability;
    EventListener callback;

    void *components[ITEM_COMPS];

} Item;

extern GameObject *item_create (u32 dbID);

extern Item *item_create_comp (u32 goID);

extern void *item_get_item_component (Item *item, ItemComponent component);
extern void item_remove_item_component (Item *item, ItemComponent component);

// TODO:
typedef enum ArmorType {

    ARMOR_HEAD,

} ArmorType;

// TODO:
typedef enum ArmorSlot {

    SLOT_HEAD,

} ArmorSlot;

// armour component for an item
typedef struct Armour {

    u32 itemID;         // item's go id
    u32 dbID;

    ArmorType type;
    ArmorSlot slot;

    u32 maxLifetime;
    u32 lifetime;
    bool isEquipped;

} Armour;

// TODO: 
typedef enum WeaponType {

    WEAPON_SWORD,

} WeaponType;

// TODO:
typedef enum WeaponSlot {

    SLOT_MAIN,

} WeaponSlot;

// TODO: add class specific weapons
// TODO: add modifiers
// weapon component for an item
typedef struct Weapon {

    u32 itemID;     
    u32 dbID;

    WeaponType type;
    WeaponSlot slot;

    u8 dps;
    u32 maxLifetime;
    u32 lifetime;
    bool isEquipped;
    bool twoHanded;

} Weapon;

#endif
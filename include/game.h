#ifndef GAME_H_
#define GAME_H_

#include "blackrock.h"

#include "map.h"    // for Point

#include "list.h"
#include "objectPool.h"


// 11/08/2018
// Getting back to an ECS but we will try to use linked lists and make a more efficient system

typedef enum GameComponent {

    POSITION = 0,
    GRAPHICS,
    PHYSICS,
    HEALTH, 
    MOVEMENT,
    COMBAT,
    ITEM, 
    EQUIPMENT, 
    TREASURE,
    ANIMATION,

    COMP_COUNT

} GameComponent;

// Entity
// TODO: do we want graphics and position to be already a part of the Go?
typedef struct {
    
    i32 id;
    void *components[COMP_COUNT];

} GameObject;


/*** COMPONENTS ***/

#define UNSERT_LAYER    0
#define GROUND_LAYER    1
#define MID_LAYER       2
#define AIR_LAYER       3
#define TOP_LAYER       4

typedef struct Position {

    i32 objectId;
    u8 x, y;
    u8 layer;   

} Position;

typedef struct Graphics {

    i32 objectId;
    asciiChar glyph;
    u32 fgColor;
    u32 bgColor;

} Graphics;

typedef struct Physics {

    i32 objectId;
    bool blocksMovement;
    bool blocksSight;

} Physics;

typedef struct Movement {

    i32 objectId;
    i32 speed;
    i32 frecuency;
    i32 ticksUntilNextMov;
    Point destination;
    bool hasDestination;
    bool chasingPlayer;
    i32 turnsSincePlayerSeen;

} Movement;

// This are the general stats for every living entity
typedef struct Stats {

    u32 health;   // base health
    u32 power;  // this represents the mana or whatever
    u32 powerRegen; // regen power/(ticks or turns)
    u32 strenght; // this modifies the damage dealt 

} Stats;

typedef struct Attack {

    u32 hitchance;      // chance to not miss the target
    u32 attackPower;    // an ogre has a higher attack power than a kobolde
    u32 damage;     // based on attack power, weapon, class, etc.
    u32 attackSpeed;    // how many hits per turn
    u32 spellPower;     // similar to attack power but for mages, etc
    u32 criticalStrike;     // chance to hit a critical (2x more powerful than normal)

} Attack;

typedef struct Defense {

    u32 armor;  // based on level, class, and equipment
    u32 dodge;  // dodge chance -> everyone can dodge
    u32 parry;  // parry chance -> only works with certain weapons and classes
    u32 block;  // block chance -> this only works with a certain class than can handle shields

} Defense;

typedef struct Combat  {

	i32 objectId;	
    Stats baseStats;	
    Attack attack;
    Defense defense;

} Combat;

// TODO: how can we handle consumables?
// TODO: how can we handle crafting?
typedef struct Item {

    i32 objectId;
    // char *name;  // TODO: do we want the name here?
    i32 type;   // epic, rare, common, rubish, etc.
    i32 quantity;   // this is used to handle stacks
    i32 weight;     // we have a max weight that we can carry based on our class, genre, etc
    i32 lifetime;
    // the dps + the attack power give the damage that we dealth in a hit
    u32 dps;    // maybe we want to be able to hit with everythig that we have on hand
    char *slot;
    bool isEquipped;

} Item;


/*** OUR LISTS ***/

extern List *gameObjects;
extern List *positions;
extern List *graphics;
extern List *physics;

/*** OUR POOLS ***/

extern Pool *goPool;
extern Pool *posPool;
extern Pool *graphicsPool;
extern Pool *physPool;


/*** GAME STATE ***/

extern void initGame (void);

extern bool playerTookTurn;

extern void updateGame (void);

/*** PLAYER ***/

// TODO: player name??
// global GameObject *player = NULL;
extern GameObject *player;


/*** Game Objects ***/

extern GameObject *createGO (void);
void *getComponent (GameObject *, GameComponent);
void addComponent (GameObject *go, GameComponent type, void *data);


/*** LEVEL MANAGER ***/

typedef struct {

    unsigned int levelNum;

    // 12/08/2018 -- 19:34 -- we will only worry for now for generating levels inside the dungeons
    // later we will want to generate levels in caves or forests, etc
    bool **mapCells;

    // 15/08/2018 -- 7:42
    // Wall ***walls;

} Level;

global Level *currentLevel;

extern unsigned int wallCount;


/*** MOVEMENT ***/

extern bool canMove (Position);


/*** ITEMS ***/

extern void getItem ();


// Cleanning Up!
extern void cleanUpGame (void);


#endif
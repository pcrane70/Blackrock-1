#ifndef GAME_H_
#define GAME_H_

#include "blackrock.h"

#include "map.h"    // for Point

#include "utils/list.h"
#include "objectPool.h"

/*** MESSAGE COLORS ***/

#define SUCCESS_COLOR   0x009900FF
#define WARNING_COLOR   0x990000FF

#define HIT_COLOR       0xF2F2F2FF  // succesfull attack
#define CRITICAL_COLOR  0xFFDB22FF  // critical hit
#define MISS_COLOR      0xCCCCCCFF  // missed attack
#define STOPPED_COLOR   0xFBFBFBFF  // parry, dodge, block
#define KILL_COLOR      0xFF9900FF  // you kill a mob

#define HEALTH_COLOR    0x00FF22FF  // player gains health
#define DAMAGE_COLOR    0x8C2020FF  // player loses health


// 11/08/2018
// Getting back to an ECS but we will try to use linked lists and make a more efficient system

typedef enum GameComponent {

    POSITION = 0,
    GRAPHICS,
    PHYSICS,
    MOVEMENT,
    COMBAT,
    ITEM, 
    EVENT,
    PLAYER,

    COMP_COUNT

} GameComponent;

typedef struct GameObject {
    
    u32 id;
    void *components[COMP_COUNT];

} GameObject;


/*** COMPONENTS ***/

#define UNSET_LAYER     0
#define GROUND_LAYER    1
#define LOWER_LAYER     2
#define MID_LAYER       3
#define TOP_LAYER       4

typedef struct Position {

    u32 objectId;
    u8 x, y;
    u8 layer;   

} Position;

typedef struct Graphics {

    u32 objectId;
    asciiChar glyph;
    u32 fgColor;
    u32 bgColor;
    bool hasBeenSeen;
    bool visibleOutsideFov;
    char *name;     // 19/08/2018 -- 16:47

} Graphics;

typedef struct Physics {

    u32 objectId;
    bool blocksMovement;
    bool blocksSight;

} Physics;

typedef struct Movement {

    u32 objectId;
    u32 speed;
    u32 frecuency;
    u32 ticksUntilNextMov;
    Point destination;
    bool hasDestination;
    bool chasingPlayer;
    u32 turnsSincePlayerSeen;

} Movement;

/*** COMBAT ***/

// This are the general stats for every living entity
typedef struct Stats {

    u32 maxHealth;   // base health
    i32 health;
    u32 power;  // this represents the mana or whatever
    u32 powerRegen; // regen power/(ticks or turns)
    u32 strength; // this modifies the damage dealt 

} Stats;

typedef struct Attack {

    u32 hitchance;      // chance to not miss the target
    u32 baseDps;        // this is mostly for npcs
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

	u32 objectId;	
    Stats baseStats;	
    Attack attack;
    Defense defense;

} Combat;


/*** EVENTS ***/

typedef void (*EventListener)(void);

typedef struct Event {

    u32 objectId;
    EventListener callback;

} Event;


/*** OUR LISTS ***/

extern List *gameObjects;
extern List *positions;
extern List *graphics;
extern List *physics;

/*** POOLS ***/

extern Pool *goPool;
extern Pool *posPool;
extern Pool *graphicsPool;
extern Pool *physPool;


/*** GAME STATE ***/

extern void initGame (void);

extern bool playerTookTurn;

extern void updateGame (void);


/*** PLAYER ***/

typedef enum CharClass {

    WARRIOR = 1,
    PALADIN,
    ROGUE,
    PRIEST,
    DEATH_KNIGHT,
    MAGE

} CharClass;

// As of 18/08/2018 -- 23:00 -- we will treat this as an independent component, not as a list
typedef struct Player {

    char *name;
    u8 genre;     // 0 female, 1 male
    // TODO: races
    CharClass cClass;
    u16 color;  // for accessibility
    u8 level;
    u16 money [3];  // gold, silver, copper
    u16 maxWeight;
    List *inventory;

} Player;

extern GameObject *player;
extern Player *playerComp;  // for accessibility


/*** Game Objects ***/

extern GameObject *createGO (void);
extern void *getComponent (GameObject *, GameComponent);
extern void addComponent (GameObject *go, GameComponent type, void *data);
extern List *getObjectsAtPos (u32 x, u32 y);

/*** LEVEL MANAGER ***/

typedef struct {

    unsigned int levelNum;

    // 12/08/2018 -- 19:34 -- we will only worry for now for generating levels inside the dungeons
    // later we will want to generate levels in caves or forests, etc
    bool **mapCells;

} Level;

global Level *currentLevel;

extern unsigned int wallCount;


/*** MOVEMENT ***/

extern bool canMove (Position);
extern bool recalculateFov;


/*** COMBAT ***/

extern void fight (GameObject *attacker, GameObject *defender);


// Cleanning Up!
extern void cleanUpGame (void);


#endif
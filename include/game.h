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


// Cleanning Up!
extern void cleanUpGame (void);


#endif
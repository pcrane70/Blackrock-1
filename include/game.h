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

global List *gameObjects;
global List *positions;
global List *graphics;
global List *physics;


/*** OUR POOLS ***/

global Pool *goPool;
global Pool *posPool;
global Pool *graphicsPool;
global Pool *physPool;


// Wolrd State
extern void initWorld (void);


/*** PLAYER ***/

// TODO: player name??
global GameObject *player = NULL;
extern GameObject *initPlayer (void);


/*** Game Objects ***/

extern GameObject *createGO (void);
void *getComponent (GameObject *, GameComponent);
void addComponent (GameObject *go, GameComponent type, void *data);


// Cleanning Up!
extern unsigned int cleanUpGame (void);


#endif
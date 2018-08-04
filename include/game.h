#ifndef GAME_H_
#define GAME_H_

#include "blackrock.h"

/*** TEST ENTITY COMPONENT SYSTEM ***/
// these are for testing our first ECS
// TODO: do we want these declartions here??

#define MAX_COMP_COUNT      20

typedef enum GameComponent {

    POSITION = 0,
    GRAPHICS,
    PHYSICS,   
    HEALTH, 
    MOVEMENT,

    COMPONENT_COUNT

} GameComponent;

// Our Entity
typedef struct GameObject {

    u32 id;
    void *components [MAX_COMP_COUNT];

} GameObject;

// TODO: do we need an object id in every component??
// or maybe just a like to the GameObject??
// or maybe this is right because we don't want every GO to have every component??

typedef struct Position {

    u32 objectId;
    u8 x, y;

} Position;

typedef struct Graphics {

    u32 objectId;
    asciiChar glyph;
    u32 fgColor;
    u32 bgColor;

} Graphics;

typedef struct Physical {

    u32 objectId;
    bool blocksMovement;
    bool blocksSight;

} Physics;


// TODO: is it better to have an array or a list??
# define MAX_GO     1000

extern GameObject gameObjects[MAX_GO];
 
// // TODO: do we want this?? is there a better way around??
extern Position positionComps[MAX_GO];
extern Graphics graphicComps[MAX_GO];
extern Physics physicsComps[MAX_GO];


extern GameObject *createGameObject ();
extern void addComponentToGO (GameObject *, GameComponent, void *);
void *getComponent (GameObject *, GameComponent);
extern void destroyGO (GameObject *);

#endif
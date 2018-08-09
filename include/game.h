#ifndef GAME_H_
#define GAME_H_

#include "blackrock.h"

// 08/08/2018
// Creating a new representation of GameObject that will be used with a linked list
// lets test if this is a better way of handling the GOs

typedef struct GameObject {

    // u32 id;

    u8 x, y;

    asciiChar glyph;
    u32 fgColor;
    u32 bgColor;

    bool blocksMovement;
    bool blocksSight;

    struct GameObject *next;

} GameObject;

// Linked Lists
extern GameObject *createGOList (GameObject *first, GameObject *data);
extern void createGO (GameObject *data);
extern void destroyGO (GameObject *go);
extern GameObject *cleanGameObjects (GameObject *first);


#endif
// FOV = Field of View
/* This is an experimental function to simulate a player that is inside a dungeon
    and there are no lights, so this can simulate the player holding a torch
    to see where he is going. */

// As of 08/08/2018 this function is not ready to be implementd on the game and this is just for testing

#include <string.h>

#include "blackrock.h"

#define FOV_DISTANCE    8

typedef struct FovCell {

    u32 x, y;

} FovCell;

typedef struct Shadow {

    float startSlope;
    float endSlope;

} Shadow;

static u8 shadoCount = 0;

void calculateFov (short unsigned int xPos, short unsigned int yPos, short unsigned int **fovmap) {

    // default state
    // FIXME: does this work properly?
    memset (fovmap, 0, sizeof (fovmap));

    // mark the position as visible
    fovmap[xPos][yPos] = 1;

    // loop through all 8 sectors around the pos
    // for (short unsigned int i = 1; i <= 8; i++) {
    //     bool prevBlocking = false;
    //     shadoCount = 0;
    //     float shadowStart = 0.0;
    //     float shadowEnd = 0.0;

    //     for (short unsigned int cellY = 1; cellY < FOV_DISTANCE; cellY++) {
    //         prevBlocking = false;
    //         for (short unsigned int cellX = 0; cellX <= cellY; cellX++) {
    //             // translate cellX, cellY to map coordinates
    //             FovCell heroCell = { posX, posY };
    //         }
    //     }
    // }

}
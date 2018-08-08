// FOV = Field of View
/* This is an experimental function to simulate a player that is inside a dungeon
    and there are no lights, so this can simulate the player holding a torch
    to see where he is going. */

// As of 08/08/2018 this function is not ready to be implementd on the game and this is just for testing

#include <string.h>

#include "blackrock.h"

#define FOV_DISTANCE    5

void calculateFov (short unsigned int x, short unsigned int y, short unsigned int **fovmap) {

    // default state
    // FIXME: does this work properly?
    memset (fovmap, 0, sizeof (fovmap));

    // cast visibility in our 4 directions
    short unsigned int x1 = 0;
    if (x >= FOV_DISTANCE) x1 = x - FOV_DISTANCE;

    short unsigned int x2 = x + FOV_DISTANCE;
    if (x2 >= MAP_WIDTH) x2 = MAP_WIDTH - 1;

    short unsigned int y1 = 0;
    if (y >= FOV_DISTANCE) y1 = y - FOV_DISTANCE;

    short unsigned int y2 = y + FOV_DISTANCE;
    if (y2 >= MAP_HEIGHT) y2 = MAP_HEIGHT - 1;

    // apply the visibility to the fov map
    for (short unsigned int fx = x1; fx <= x2; fx++)
        for (short unsigned int fy = y1; fy <= y2; fy++)
            fovmap[fx][fy] = 10;

}
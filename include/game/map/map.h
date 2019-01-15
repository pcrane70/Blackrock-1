#ifndef MAP_H_
#define MAP_H_

#include "game.h"

// 05/08/2018 -- 16:28
// TODO: do we really need a coord system for a better programming experience??
// also think about how do we want to implement open areas later??
typedef struct Point {

    i32 x, y;

}  Point;

typedef struct Segment {

    Point start, mid, end;
    i32 roomFrom, roomTo;
    bool hasWayPoint;

} Segment;

// TODO: lets try with this number...
#define MAX_WALLS   2000

// 19/08/2018 -- 17:51 -- we are not adding layers to the wall beacuse we render them
// slightly differently
typedef struct Wall {

    u32 x, y; // position

    // graphics
    // asciiChar glyph;
    bool hasBeenSeen;
    // bool visibleOutsideFov;  

    // physics
    bool blocksMovement;
    bool blocksSight;

} Wall;

// TODO: we are testing having the walls in a separte array in memory for conviniece
// for the other systems tha we want to implement in the other gameObjects...
// extern Wall walls[MAP_WIDTH][MAP_HEIGHT];

extern Wall walls[MAX_WALLS];

extern void initMap (bool **mapCells);
extern Point getFreeSpot (bool **mapCells);

#endif
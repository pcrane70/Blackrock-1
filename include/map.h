#ifndef MAP_H_
#define MAP_H_

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

typedef struct Wall {

    // FIXME: do we need an id?
    // u32 id;

    u8 x, y; // position

    // graphics
    asciiChar glyph;
    u32 fgColor;
    u32 bgColor;

    // physics
    bool blocksMovement;
    bool blocksSight;

} Wall;

extern Wall walls[MAX_WALLS];

extern unsigned int initWorld (GameObject *player);

#endif
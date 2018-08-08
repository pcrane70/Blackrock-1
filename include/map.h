#ifndef MAP_H_
#define MAP_H_

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
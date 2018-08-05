/*** MAP ***/

#include "game.h"
#include "console.h"

#include "myUtils.h"

// TODO: how much space do we want to leave for the HUD??
#define MAP_WIDTH   80
#define MAP_HEIGHT  40

// 05/08/2018 -- 16:28
// TODO: do we really need a coord system for a better programming experience??
// also think about how do we want to implement open areas later??
typedef struct {

    u32 x, y;

}  Point;

// TODO: is this the system that we want to go with??
// if yes, probably allocate inside the map generator, 
// we will not want this hanging out later
bool mapCells[MAP_WIDTH][MAP_HEIGHT];

/*** OTHER ***/

Point randomRectPoint (Rect rect) {

    u32 px = (rect.x, (rect.x + rect.w));
    u32 py = (rect.y, (rect.y + rect.h));

    Point randPoint = { px, py };
    return randPoint;

}

/*** CARVING ***/

bool carveRoom (u32 x, u32 y, u32 w, u32 h) {

    // checks if it overlaps another room
    // and also check that their are not to close together
    for (u8 i = x; i < x + (w + 1); i++) 
        for (u8 j = y; j < y + (h + 1); j++)
            if (mapCells[i][j] == false) return false;

    // carve the room
    for (u8 i = x; i < x + w; i++) 
        for (u8 j = y; j < y + h; j++)
            mapCells[i][j] = false;

    return true;

}

void carveCorridorHor (Point from, Point to) {

    u32 first, last;
    if (from.x < to.x) {
        first = from.x;
        last = to.x;
    }

    else {
        first = to.x;
        last = from.x;
    }

    for (u32 x = first; x <= last; x++) 
        mapCells[x][from.y] = false;

}

void carveCorridorVer (Point from, Point to) {

    u32 first, last;
    if (from.y < to.y) {
        first = from.y;
        last = to.y;
    }

    else {
        first = to.y;
        last = from.y;
    }

    for (u32 y = first; y <= last; y++)
        mapCells[from.x][y] = false;

}


/*** DRAWING ***/

// TODO: what color do we want for walls?
void createWall (u32 x, u32 y) {

    GameObject *wall = createGameObject ();
    Position wallPos = { wall->id, x, y };
    addComponentToGO (wall, POSITION, &wallPos);
    Graphics wallGraphics = { wall->id, '|', 0xFFFFFFFF, 0x000000FF };
    addComponentToGO (wall, GRAPHICS, &wallGraphics);
    Physics wallPhysics = { wall->id, true, true };
    addComponentToGO (wall, PHYSICS, &wallPhysics);

}

void generateMap () {

    // mark all the cells as filled
    for (u32 x = 0; x < MAP_WIDTH; x++) 
        for (u32 y = 0; y < MAP_HEIGHT; y++) 
            mapCells[true];

    // carve out non-overlaping rooms that are randomly placed, 
    // and of random size
    bool roomsDone = false;
    // FIXME: do we want to allocate so much memory like this?
    Rect rooms[100];
    u32 roomCount = 0;

    u32 cellsUsed = 0;
    
    // create room data
    while (!roomsDone) {
        // generate a random width/height for a room
        u32 w = (u32) randomInt (4, 12);
        u32 h = (u32) randomInt (4, 12);

        // generate random positions
        u32 x = (u32) randomInt (1, MAP_WIDTH - w - 1);
        u32 y = (u32) randomInt (1, MAP_HEIGHT - h - 1);

        if (carveRoom (x, y, w, h)) {
            Rect rect = { x, y, w, h };
            rooms[roomCount] = rect;
            roomCount++;
            cellsUsed += (w * h);
        }

        if (((float) cellsUsed / (float) (MAP_HEIGHT * MAP_WIDTH)) > 0.45) roomsDone = true;

    }

    // TODO: draw rooms
    
    // This is a simple way of drawing corridors, is this a good way, 
    // or do we need a more advanced system??
    // join all the rooms with corridors
    for (u32 r = 1; r < roomCount; r++) {
        Rect from = rooms[r - 1];
        Rect to = rooms[r];

        Point fromPt = randomRectPoint (from);
        Point toPt = randomRectPoint (to);

        // simple way of making corridors
        // TODO: do we want a more complex method?
        // think in project prourcopine pathfinding...
        if (randomInt (0, 1) == 0) {
            // move horizontal, then vertical
            Point midPt = { toPt.x, fromPt.y };
            carveCorridorHor (fromPt, midPt);
            carveCorridorVer (midPt, toPt);
        }

        else {
            // move vertical, then horizontal
            Point midPt = { fromPt.x, toPt.y };
            carveCorridorVer (fromPt, midPt);
            carveCorridorHor (midPt, toPt);
        }
    }

    // and make sure that all of them are reachable

}